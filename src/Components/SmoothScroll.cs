using System;
using Avalonia;
using Avalonia.Animation;
using Avalonia.Animation.Easings;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Styling;

namespace frontend_avalonia.Components
{
    /// <summary>
    /// Attached behaviour that adds physics-style smooth (inertial) scrolling to any ScrollViewer.
    /// Usage in XAML:  local:SmoothScroll.IsEnabled="True"
    /// </summary>
    public static class SmoothScroll
    {
        // ── Attached property ────────────────────────────────────────────────

        public static readonly AttachedProperty<bool> IsEnabledProperty =
            AvaloniaProperty.RegisterAttached<ScrollViewer, bool>(
                "IsEnabled",
                typeof(SmoothScroll),
                defaultValue: false,
                inherits: false);

        public static bool GetIsEnabled(ScrollViewer element) =>
            element.GetValue(IsEnabledProperty);

        public static void SetIsEnabled(ScrollViewer element, bool value) =>
            element.SetValue(IsEnabledProperty, value);

        // ── Wiring ───────────────────────────────────────────────────────────

        static SmoothScroll()
        {
            IsEnabledProperty.Changed.AddClassHandler<ScrollViewer>(OnIsEnabledChanged);
        }

        private static void OnIsEnabledChanged(ScrollViewer viewer, AvaloniaPropertyChangedEventArgs e)
        {
            if (e.NewValue is true)
            {
                // Attach a per-viewer state object so multiple viewers don't share state
                var state = new ScrollState(viewer);
                viewer.PointerWheelChanged += state.OnPointerWheelChanged;
                // Clean up when the viewer is detached from the visual tree
                viewer.DetachedFromVisualTree += (_, _) =>
                {
                    viewer.PointerWheelChanged -= state.OnPointerWheelChanged;
                    state.Dispose();
                };
            }
        }

        // ── Per-viewer state ─────────────────────────────────────────────────

        private sealed class ScrollState : IDisposable
        {
            // How many pixels one notch scrolls (before easing)
            private const double PixelsPerNotch = 120.0;
            // Duration of the easing animation in ms
            private const double AnimationMs = 380.0;

            private readonly ScrollViewer _viewer;
            private readonly QuinticEaseOut _easing = new();

            // Running animation state
            private double _targetOffset;
            private double _startOffset;
            private double _animProgress;   // 0..1
            private bool   _animating;
            private DateTimeOffset _animStartTime;
            private bool _disposed;

            public ScrollState(ScrollViewer viewer)
            {
                _viewer = viewer;
                _targetOffset = viewer.Offset.Y;
            }

            internal void OnPointerWheelChanged(object? sender, PointerWheelEventArgs e)
            {
                // Accumulate target — keeps momentum when wheel is spun fast
                double delta = -e.Delta.Y * PixelsPerNotch;
                double maxOffset = Math.Max(0.0, _viewer.Extent.Height - _viewer.Viewport.Height);
                _targetOffset = Math.Clamp(
                    _targetOffset + delta,
                    0.0,
                    maxOffset);

                // Start a new animation from current position
                _startOffset    = _viewer.Offset.Y;
                _animProgress   = 0;
                _animStartTime  = DateTimeOffset.UtcNow;

                if (!_animating)
                {
                    _animating = true;
                    // Use Avalonia's rendering loop via a DispatcherTimer tick
                    StartAnimationLoop();
                }

                // Prevent the default jump-scroll
                e.Handled = true;
            }

            private void StartAnimationLoop()
            {
                var timer = new Avalonia.Threading.DispatcherTimer
                {
                    Interval = TimeSpan.FromMilliseconds(8) // ~120fps
                };
                timer.Tick += (_, _) =>
                {
                    if (_disposed) { timer.Stop(); return; }

                    double elapsed = (DateTimeOffset.UtcNow - _animStartTime).TotalMilliseconds;
                    _animProgress = Math.Min(elapsed / AnimationMs, 1.0);

                    double easedProgress = _easing.Ease(_animProgress);
                    double newY = _startOffset + (_targetOffset - _startOffset) * easedProgress;

                    _viewer.Offset = new Vector(_viewer.Offset.X, newY);

                    if (_animProgress >= 1.0)
                    {
                        _animating = false;
                        timer.Stop();
                    }
                };
                timer.Start();
            }

            public void Dispose()
            {
                _disposed = true;
            }
        }
    }
}

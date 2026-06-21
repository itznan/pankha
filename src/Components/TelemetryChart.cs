using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Globalization;
using System.Linq;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Media;
using Avalonia.Styling;

namespace frontend_avalonia.Components
{
    public class TelemetryChart : Control
    {
        public static readonly StyledProperty<ObservableCollection<double>?> RpmHistoryProperty =
            AvaloniaProperty.Register<TelemetryChart, ObservableCollection<double>?>(
                nameof(RpmHistory),
                null);

        public static readonly StyledProperty<ObservableCollection<double>?> TempHistoryProperty =
            AvaloniaProperty.Register<TelemetryChart, ObservableCollection<double>?>(
                nameof(TempHistory),
                null);

        static TelemetryChart()
        {
            AffectsRender<TelemetryChart>(RpmHistoryProperty, TempHistoryProperty);
        }

        public TelemetryChart()
        {
            this.ActualThemeVariantChanged += (s, e) => InvalidateVisual();
        }

        public ObservableCollection<double>? RpmHistory
        {
            get => GetValue(RpmHistoryProperty);
            set => SetValue(RpmHistoryProperty, value);
        }

        public ObservableCollection<double>? TempHistory
        {
            get => GetValue(TempHistoryProperty);
            set => SetValue(TempHistoryProperty, value);
        }

        protected override void OnPropertyChanged(AvaloniaPropertyChangedEventArgs change)
        {
            base.OnPropertyChanged(change);

            if (change.Property == RpmHistoryProperty)
            {
                if (change.OldValue is INotifyCollectionChanged oldRpm)
                    oldRpm.CollectionChanged -= OnHistoryCollectionChanged;
                if (change.NewValue is INotifyCollectionChanged newRpm)
                    newRpm.CollectionChanged += OnHistoryCollectionChanged;
            }

            if (change.Property == TempHistoryProperty)
            {
                if (change.OldValue is INotifyCollectionChanged oldTemp)
                    oldTemp.CollectionChanged -= OnHistoryCollectionChanged;
                if (change.NewValue is INotifyCollectionChanged newTemp)
                    newTemp.CollectionChanged += OnHistoryCollectionChanged;
            }
        }

        private void OnHistoryCollectionChanged(object? sender, NotifyCollectionChangedEventArgs e)
        {
            InvalidateVisual();
        }

        public override void Render(DrawingContext context)
        {
            base.Render(context);

            var bounds = Bounds;
            double width = bounds.Width;
            double height = bounds.Height;

            bool isDark = true;

            // Colors
            Color chartBgColor = isDark ? Color.FromRgb(36, 36, 38) : Color.FromRgb(230, 230, 232);
            Color gridColor = isDark ? Color.FromRgb(50, 50, 52) : Color.FromRgb(215, 215, 217);
            Color labelColor = isDark ? Color.FromRgb(130, 130, 134) : Color.FromRgb(120, 120, 124);
            Color rpmColor = Color.FromRgb(10, 132, 255);       // macOS system blue
            Color tempColor = Color.FromRgb(255, 159, 10);     // macOS system orange

            // Draw Background panel
            var backgroundBrush = new SolidColorBrush(chartBgColor);
            context.DrawRectangle(backgroundBrush, null, new Rect(0, 0, width, height), 8, 8);

            double paddingLeft = 45;
            double paddingRight = 45;
            double paddingTop = 25;
            double paddingBottom = 20;

            double w = width - paddingLeft - paddingRight;
            double h = height - paddingTop - paddingBottom;

            var rpmList = RpmHistory?.ToList() ?? new List<double>();
            var tempList = TempHistory?.ToList() ?? new List<double>();

            if (rpmList.Count == 0 || w <= 0 || h <= 0)
            {
                // Draw empty message
                var typeface = new Typeface("Inter");
                var formattedText = new FormattedText(
                    "Waiting for sensor telemetry...",
                    CultureInfo.CurrentCulture,
                    FlowDirection.LeftToRight,
                    typeface,
                    12,
                    new SolidColorBrush(labelColor));
                
                context.DrawText(formattedText, new Point((width - formattedText.Width) / 2, (height - formattedText.Height) / 2));
                return;
            }

            // Determine scale for RPM (dynamic)
            double maxRpm = 1000.0;
            foreach (double r in rpmList)
            {
                if (r > maxRpm) maxRpm = r;
            }
            maxRpm = ((int)(maxRpm + 499) / 500) * 500; // round to nearest 500 RPM

            // Draw horizontal grid lines
            var gridPen = new Pen(new SolidColorBrush(gridColor), 1);
            for (int i = 1; i <= 3; ++i)
            {
                double y = paddingTop + (i * h / 4);
                context.DrawLine(gridPen, new Point(paddingLeft, y), new Point(paddingLeft + w, y));
            }

            // Draw Y axis labels
            var fontTypeface = new Typeface("Inter");
            var textBrush = new SolidColorBrush(labelColor);

            // Left Y Axis (RPM)
            var rpmLabels = new[] { maxRpm.ToString("F0"), (maxRpm / 2).ToString("F0"), "0" };
            double[] yPositions = new[] { paddingTop, paddingTop + h / 2, paddingTop + h };

            for (int i = 0; i < 3; i++)
            {
                var formattedText = new FormattedText(
                    rpmLabels[i],
                    CultureInfo.CurrentCulture,
                    FlowDirection.LeftToRight,
                    fontTypeface,
                    9,
                    textBrush);
                context.DrawText(formattedText, new Point(paddingLeft - formattedText.Width - 5, yPositions[i] - formattedText.Height / 2));
            }

            // Right Y Axis (Temperature)
            var tempLabels = new[] { "100°C", "50°C", "0°C" };
            for (int i = 0; i < 3; i++)
            {
                var formattedText = new FormattedText(
                    tempLabels[i],
                    CultureInfo.CurrentCulture,
                    FlowDirection.LeftToRight,
                    fontTypeface,
                    9,
                    textBrush);
                context.DrawText(formattedText, new Point(paddingLeft + w + 5, yPositions[i] - formattedText.Height / 2));
            }

            // Render Data Lines
            int maxHistoryPoints = 60; // match C++
            int size = rpmList.Count;

            var rpmPoints = new List<Point>();
            var tempPoints = new List<Point>();

            for (int i = 0; i < size; ++i)
            {
                double x = paddingLeft + (i * w / (maxHistoryPoints - 1));
                
                double rpmVal = rpmList[i];
                double tempVal = tempList[i];
                
                double rpmY = paddingTop + h - (rpmVal * h / maxRpm);
                double tempY = paddingTop + h - (tempVal * h / 100.0);

                // Bound checks
                rpmY = Math.Clamp(rpmY, paddingTop, paddingTop + h);
                tempY = Math.Clamp(tempY, paddingTop, paddingTop + h);

                rpmPoints.Add(new Point(x, rpmY));
                tempPoints.Add(new Point(x, tempY));
            }

            // Draw fills
            if (rpmPoints.Count > 1)
            {
                // RPM Area Fill
                var rpmFillGeom = new StreamGeometry();
                using (var ctx = rpmFillGeom.Open())
                {
                    ctx.BeginFigure(new Point(paddingLeft, paddingTop + h), true);
                    foreach (var pt in rpmPoints)
                    {
                        ctx.LineTo(pt);
                    }
                    ctx.LineTo(new Point(rpmPoints.Last().X, paddingTop + h));
                    ctx.EndFigure(true);
                }
                var rpmFillBrush = new LinearGradientBrush
                {
                    StartPoint = new RelativePoint(0, 0, RelativeUnit.Relative),
                    EndPoint = new RelativePoint(0, 1, RelativeUnit.Relative)
                };
                rpmFillBrush.GradientStops.Add(new GradientStop(Color.FromArgb(30, rpmColor.R, rpmColor.G, rpmColor.B), 0));
                rpmFillBrush.GradientStops.Add(new GradientStop(Color.FromArgb(0, rpmColor.R, rpmColor.G, rpmColor.B), 1));
                context.DrawGeometry(rpmFillBrush, null, rpmFillGeom);

                // Temp Area Fill
                var tempFillGeom = new StreamGeometry();
                using (var ctx = tempFillGeom.Open())
                {
                    ctx.BeginFigure(new Point(paddingLeft, paddingTop + h), true);
                    foreach (var pt in tempPoints)
                    {
                        ctx.LineTo(pt);
                    }
                    ctx.LineTo(new Point(tempPoints.Last().X, paddingTop + h));
                    ctx.EndFigure(true);
                }
                var tempFillBrush = new LinearGradientBrush
                {
                    StartPoint = new RelativePoint(0, 0, RelativeUnit.Relative),
                    EndPoint = new RelativePoint(0, 1, RelativeUnit.Relative)
                };
                tempFillBrush.GradientStops.Add(new GradientStop(Color.FromArgb(30, tempColor.R, tempColor.G, tempColor.B), 0));
                tempFillBrush.GradientStops.Add(new GradientStop(Color.FromArgb(0, tempColor.R, tempColor.G, tempColor.B), 1));
                context.DrawGeometry(tempFillBrush, null, tempFillGeom);

                // Draw Lines
                var rpmStrokeGeom = new StreamGeometry();
                using (var ctx = rpmStrokeGeom.Open())
                {
                    ctx.BeginFigure(rpmPoints[0], false);
                    for (int i = 1; i < rpmPoints.Count; i++)
                    {
                        ctx.LineTo(rpmPoints[i]);
                    }
                    ctx.EndFigure(false);
                }
                context.DrawGeometry(null, new Pen(new SolidColorBrush(rpmColor), 2), rpmStrokeGeom);

                var tempStrokeGeom = new StreamGeometry();
                using (var ctx = tempStrokeGeom.Open())
                {
                    ctx.BeginFigure(tempPoints[0], false);
                    for (int i = 1; i < tempPoints.Count; i++)
                    {
                        ctx.LineTo(tempPoints[i]);
                    }
                    ctx.EndFigure(false);
                }
                context.DrawGeometry(null, new Pen(new SolidColorBrush(tempColor), 2), tempStrokeGeom);
            }
        }
    }
}

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
using frontend_avalonia.Services;

namespace frontend_avalonia.Components
{
    public class CalibrationChart : Control
    {
        public static readonly StyledProperty<ObservableCollection<RegistrySettingsManager.CurvePoint>?> PointsProperty =
            AvaloniaProperty.Register<CalibrationChart, ObservableCollection<RegistrySettingsManager.CurvePoint>?>(
                nameof(Points),
                null);

        static CalibrationChart()
        {
            AffectsRender<CalibrationChart>(PointsProperty);
        }

        public CalibrationChart()
        {
            this.ActualThemeVariantChanged += (s, e) => InvalidateVisual();
        }

        public ObservableCollection<RegistrySettingsManager.CurvePoint>? Points
        {
            get => GetValue(PointsProperty);
            set => SetValue(PointsProperty, value);
        }

        protected override void OnPropertyChanged(AvaloniaPropertyChangedEventArgs change)
        {
            base.OnPropertyChanged(change);

            if (change.Property == PointsProperty)
            {
                if (change.OldValue is INotifyCollectionChanged oldPoints)
                    oldPoints.CollectionChanged -= OnPointsCollectionChanged;
                if (change.NewValue is INotifyCollectionChanged newPoints)
                    newPoints.CollectionChanged += OnPointsCollectionChanged;
            }
        }

        private void OnPointsCollectionChanged(object? sender, NotifyCollectionChangedEventArgs e)
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
            Color chartBgColor = isDark ? Color.FromRgb(30, 30, 30) : Color.FromRgb(240, 240, 242);
            Color axisColor = isDark ? Color.FromRgb(80, 80, 80) : Color.FromRgb(180, 180, 180);
            Color gridColor = isDark ? Color.FromRgb(50, 50, 50) : Color.FromRgb(225, 225, 227);
            Color labelColor = isDark ? Color.FromRgb(150, 150, 150) : Color.FromRgb(100, 100, 100);
            Color lineColor = Color.FromRgb(16, 185, 129); // Premium emerald green
            Color dotColor = Color.FromRgb(0, 122, 255);   // Premium macOS blue

            // Draw Background panel
            var backgroundBrush = new SolidColorBrush(chartBgColor);
            context.DrawRectangle(backgroundBrush, null, new Rect(0, 0, width, height));

            double padding = 40;
            double w = width - 2 * padding;
            double h = height - 2 * padding;

            var pointsList = Points?.ToList() ?? new List<RegistrySettingsManager.CurvePoint>();

            if (pointsList.Count == 0 || w <= 0 || h <= 0)
            {
                var typeface = new Typeface("Inter");
                var formattedText = new FormattedText(
                    "No calibration data available",
                    CultureInfo.CurrentCulture,
                    FlowDirection.LeftToRight,
                    typeface,
                    12,
                    new SolidColorBrush(labelColor));
                
                context.DrawText(formattedText, new Point((width - formattedText.Width) / 2, (height - formattedText.Height) / 2));
                return;
            }

            // Find max RPM
            double maxRpm = 1000.0;
            foreach (var p in pointsList)
            {
                if (p.Speed > maxRpm) maxRpm = p.Speed;
            }
            maxRpm = ((int)(maxRpm + 499) / 500) * 500;

            var fontTypeface = new Typeface("Inter");
            var textBrush = new SolidColorBrush(labelColor);

            // Draw grid lines and labels (RPM)
            var gridPen = new Pen(new SolidColorBrush(gridColor), 1, DashStyle.Dash);
            int numGridLines = 5;
            for (int i = 0; i <= numGridLines; ++i)
            {
                double y = padding + h - (i * h / numGridLines);
                context.DrawLine(gridPen, new Point(padding, y), new Point(padding + w, y));

                int rpmVal = (int)(i * maxRpm / numGridLines);
                var formattedText = new FormattedText(
                    rpmVal.ToString(),
                    CultureInfo.CurrentCulture,
                    FlowDirection.LeftToRight,
                    fontTypeface,
                    9,
                    textBrush);
                context.DrawText(formattedText, new Point(padding - formattedText.Width - 10, y - formattedText.Height / 2));
            }

            // Draw grid lines and labels (Duty Cycle)
            for (int i = 0; i <= 10; ++i)
            {
                double x = padding + (i * w / 10);
                context.DrawLine(gridPen, new Point(x, padding), new Point(x, padding + h));

                var formattedText = new FormattedText(
                    $"{i * 10}%",
                    CultureInfo.CurrentCulture,
                    FlowDirection.LeftToRight,
                    fontTypeface,
                    9,
                    textBrush);
                context.DrawText(formattedText, new Point(x - formattedText.Width / 2, padding + h + 5));
            }

            // Axis lines (Solid)
            var axisPen = new Pen(new SolidColorBrush(axisColor), 2);
            context.DrawLine(axisPen, new Point(padding, padding), new Point(padding, padding + h));
            context.DrawLine(axisPen, new Point(padding, padding + h), new Point(padding + w, padding + h));

            // Plot points
            var sortedPoints = pointsList.OrderBy(p => p.Temp).ToList(); // in registry points Temp holds Duty cycle, Speed holds RPM
            var drawPoints = new List<Point>();
            foreach (var p in sortedPoints)
            {
                double x = padding + (p.Temp * w / 100.0);
                double y = padding + h - (p.Speed * h / maxRpm);
                
                x = Math.Clamp(x, padding, padding + w);
                y = Math.Clamp(y, padding, padding + h);

                drawPoints.Add(new Point(x, y));
            }

            if (drawPoints.Count > 1)
            {
                // Area Under Curve Gradient Fill
                var areaGeom = new StreamGeometry();
                using (var ctx = areaGeom.Open())
                {
                    ctx.BeginFigure(new Point(padding, padding + h), true);
                    foreach (var pt in drawPoints)
                    {
                        ctx.LineTo(pt);
                    }
                    ctx.LineTo(new Point(drawPoints.Last().X, padding + h));
                    ctx.EndFigure(true);
                }
                var fillBrush = new LinearGradientBrush
                {
                    StartPoint = new RelativePoint(0, 0, RelativeUnit.Relative),
                    EndPoint = new RelativePoint(0, 1, RelativeUnit.Relative)
                };
                fillBrush.GradientStops.Add(new GradientStop(Color.FromArgb(60, lineColor.R, lineColor.G, lineColor.B), 0));
                fillBrush.GradientStops.Add(new GradientStop(Color.FromArgb(0, lineColor.R, lineColor.G, lineColor.B), 1));
                context.DrawGeometry(fillBrush, null, areaGeom);

                // Line Stroke
                var lineGeom = new StreamGeometry();
                using (var ctx = lineGeom.Open())
                {
                    ctx.BeginFigure(drawPoints[0], false);
                    for (int i = 1; i < drawPoints.Count; i++)
                    {
                        ctx.LineTo(drawPoints[i]);
                    }
                    ctx.EndFigure(false);
                }
                context.DrawGeometry(null, new Pen(new SolidColorBrush(lineColor), 3), lineGeom);
            }

            // Dot points
            var dotBrush = new SolidColorBrush(dotColor);
            foreach (var pt in drawPoints)
            {
                context.DrawGeometry(dotBrush, null, new EllipseGeometry(new Rect(pt.X - 4, pt.Y - 4, 8, 8)));
            }
        }
    }
}

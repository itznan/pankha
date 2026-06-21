using System;
using System.Collections.ObjectModel;
using CommunityToolkit.Mvvm.ComponentModel;

namespace frontend_avalonia.ViewModels
{
    public partial class FanViewModel : ObservableObject
    {
        [ObservableProperty]
        private string _id = string.Empty;

        [ObservableProperty]
        private string _name = string.Empty;

        [ObservableProperty]
        private string _hardwareName = string.Empty;

        [ObservableProperty]
        private double _rpm;

        [ObservableProperty]
        private double _duty;

        [ObservableProperty]
        private double _tempValue;

        [ObservableProperty]
        private string _controlId = string.Empty;

        [ObservableProperty]
        private int _minRpm;

        [ObservableProperty]
        private int _maxRpm;

        [ObservableProperty]
        private string _hardwarePath = string.Empty;

        [ObservableProperty]
        private bool _isSelected;

        public ObservableCollection<double> RpmHistory { get; } = new();
        public ObservableCollection<double> TempHistory { get; } = new();

        public string RpmText => (Rpm > 0) ? $"{Rpm:F0} RPM"
                               : (MinRpm == 0 && MaxRpm == 0) ? "-- RPM"   // no tachometer
                               : "0 RPM";
        public string DutyText => $"{Duty:F0}%";
        public string TempText => TempValue > 0 ? $"{TempValue:F1}°C" : "---°C";
        public bool IsControllable => !string.IsNullOrEmpty(ControlId);

        public FanViewModel(FanController.FanInfo info)
        {
            Update(info);
        }

        public void Update(FanController.FanInfo info)
        {
            Id = info.Id;
            Name = string.IsNullOrEmpty(info.Name) ? "Unknown Fan" : info.Name;
            HardwareName = info.HardwareName;
            Rpm = info.Rpm;
            Duty = info.SpeedPercent;
            TempValue = info.TempValue;
            ControlId = info.ControlId ?? string.Empty;
            MinRpm = (int)info.Min;
            MaxRpm = (int)info.Max;
            HardwarePath = info.Id;

            // Update UI properties
            OnPropertyChanged(nameof(RpmText));
            OnPropertyChanged(nameof(DutyText));
            OnPropertyChanged(nameof(TempText));
            OnPropertyChanged(nameof(IsControllable));

            // Add history
            RpmHistory.Add(Rpm);
            TempHistory.Add(TempValue);

            const int maxPoints = 60;
            while (RpmHistory.Count > maxPoints) RpmHistory.RemoveAt(0);
            while (TempHistory.Count > maxPoints) TempHistory.RemoveAt(0);
        }

        public void ClearHistory()
        {
            RpmHistory.Clear();
            TempHistory.Clear();
        }
    }
}

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Threading.Tasks;
using Avalonia.Threading;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using frontend_avalonia.Services;

namespace frontend_avalonia.ViewModels
{
    public partial class MainWindowViewModel : ViewModelBase
    {
        private readonly FanController? _controller;
        private readonly DispatcherTimer _pollTimer;
        private readonly DispatcherTimer _calibrationTimer;
        
        // Active Registry Settings Cached
        private Dictionary<string, int> _fanModes = new();

        [ObservableProperty]
        private ObservableCollection<FanViewModel> _fans = new();

        [ObservableProperty]
        [NotifyPropertyChangedFor(nameof(HasSelectedFan))]
        [NotifyPropertyChangedFor(nameof(IsControllable))]
        private FanViewModel? _selectedFan;

        [ObservableProperty]
        private string _connectionStatusText = "Initializing...";

        [ObservableProperty]
        private string _connectionStatusColor = "#ff9f0a"; // orange

        [ObservableProperty]
        private bool _isScanning;

        [ObservableProperty]
        private bool _isSettingsOpen;



        [ObservableProperty]
        private int _pollInterval = 2000;

        [ObservableProperty]
        private bool _showRpmInStatusBar = true;

        [ObservableProperty]
        private bool _startOnBoot;

        // Current Selected Fan Control State
        [ObservableProperty]
        [NotifyPropertyChangedFor(nameof(IsManualMode))]
        private int _selectedControlModeIndex = -1; // 0 = BIOS, 1 = Manual, 2 = Curve

        public bool IsManualMode => SelectedControlModeIndex == 1;

        [ObservableProperty]
        private int _targetManualSpeed = 50;

        // Advanced (Display only)
        [ObservableProperty] private string _advHardwarePath = "---";
        [ObservableProperty] private int _advMinRpm;
        [ObservableProperty] private int _advMaxRpm;
        [ObservableProperty] private string _advControlId = "---";

        // Status bar values
        [ObservableProperty] private string _statusBarLeftText = "Initializing...";
        [ObservableProperty] private string _statusBarRightText = "";

        // Warning banner
        [ObservableProperty] private bool _showMotherboardWarning = false;
        [ObservableProperty] private string _motherboardWarningText = "";

        // Hardware-minimum warning banner (shown when chip/BIOS clamps fan speed)
        [ObservableProperty] private bool _showHardwareMinWarning = false;
        [ObservableProperty] private string _hardwareMinWarningText = "";

        // Calibration State Overlay
        [ObservableProperty] private bool _isCalibrating;
        [ObservableProperty] private string _calibrationStatus = "";
        [ObservableProperty] private string _calibrationTelemetry = "";
        [ObservableProperty] private int _calibrationProgress;
        [ObservableProperty] private bool _calibrationShowResults;
        [ObservableProperty] private string _calibrationResultsMinMax = "";
        [ObservableProperty] private ObservableCollection<RegistrySettingsManager.CurvePoint> _calibrationPoints = new();

        public bool HasSelectedFan => SelectedFan != null;
        public bool IsControllable => SelectedFan != null && SelectedFan.IsControllable;

        // Calibration logic variables
        private string _calibratingControlId = "";
        private string _calibratingFanId = "";
        private string _calibratingFanName = "";
        private int _calibInitialMode;
        private int _calibInitialDuty;
        private int _calibStepIndex;
        private int _calibTicksInStep;
        private readonly List<int> _calibSteps = new() { 100, 90, 80, 70, 60, 50, 40, 30, 20, 10, 0 };
        private double _calibCurrentTemp;
        private int _calibCurrentRpm;

        // Lock flag to prevent trigger loops during bindings
        private bool _isUpdatingUiFromSelection = false;

        public MainWindowViewModel()
        {
            // Setup regular poll timer
            _pollTimer = new DispatcherTimer();
            _pollTimer.Tick += async (s, e) => await PollFansAsync();

            // Setup calibration timer (500ms ticks)
            _calibrationTimer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(500) };
            _calibrationTimer.Tick += async (s, e) => await OnCalibrationTickAsync();

            // Load Settings
            LoadSettings();

            // Initialize in-process FanController directly
            try
            {
                _controller = new FanController();
                
                StatusBarLeftText = "Hardware controller initialized.";
                ConnectionStatusText = "Online";
                ConnectionStatusColor = "#10B981"; // green

                // Check if motherboard Super I/O sensors loaded
                if (!_controller.MotherboardSensorsDetected)
                {
                    ShowMotherboardWarning = true;
                    MotherboardWarningText = "⚠️  Motherboard fans not detected. Windows Defender blocked the hardware driver (WinRing0). " +
                        "To fix: open Windows Security → Virus & threat protection → Exclusions → Add exclusion → Folder → select the Pankha install folder. Then restart Pankha.";
                }

                // Start polling
                _pollTimer.Start();
                _ = PollFansAsync();
            }
            catch (Exception ex)
            {
                StatusBarLeftText = $"Initialization Error: LibreHardwareMonitor requires Administrator privileges. Run application as administrator. Details: {ex.Message}";
                ConnectionStatusText = "Error";
                ConnectionStatusColor = "#ff3b30"; // red
            }
        }

        private void LoadSettings()
        {
            ShowRpmInStatusBar = RegistrySettingsManager.LoadBool("showRpm", true);
            PollInterval = RegistrySettingsManager.LoadInt("pollInterval", 2000);
            StartOnBoot = RegistrySettingsManager.LoadBool("startOnBoot", false);

            _fanModes = RegistrySettingsManager.LoadFanModes();
            
            _pollTimer.Interval = TimeSpan.FromMilliseconds(PollInterval);
        }

        private void SaveSettings()
        {
            RegistrySettingsManager.SaveValue("showRpm", ShowRpmInStatusBar);
            RegistrySettingsManager.SaveValue("pollInterval", PollInterval);
            RegistrySettingsManager.SaveValue("startOnBoot", StartOnBoot);
            RegistrySettingsManager.SetStartOnBoot(StartOnBoot);
        }

        partial void OnShowRpmInStatusBarChanged(bool value)
        {
            UpdateStatusBarRpmText();
            SaveSettings();
        }

        private void UpdateStatusBarRpmText()
        {
            if (ShowRpmInStatusBar)
            {
                var controllableRpmList = Fans.Select(f => $"{f.Name}: {f.RpmText}");
                StatusBarRightText = string.Join("  |  ", controllableRpmList);
            }
            else
            {
                StatusBarRightText = "";
            }
        }

        [RelayCommand]
        public async Task RescanAsync()
        {
            if (IsScanning || _controller == null) return;
            IsScanning = true;
            StatusBarLeftText = "Scanning hardware sensors...";

            try
            {
                await PollFansAsync();
                StatusBarLeftText = "Scan completed.";
            }
            catch (Exception ex)
            {
                StatusBarLeftText = $"Scan failed: {ex.Message}";
            }
            finally
            {
                IsScanning = false;
            }
        }

        [RelayCommand]
        public void ToggleSettings()
        {
            IsSettingsOpen = !IsSettingsOpen;
            if (!IsSettingsOpen)
            {
                // Saved immediately on close
                SaveSettings();
                _pollTimer.Interval = TimeSpan.FromMilliseconds(PollInterval);
            }
        }

        [RelayCommand]
        public void DismissMotherboardWarning()
        {
            ShowMotherboardWarning = false;
        }

        [RelayCommand]
        public void DismissHardwareMinWarning()
        {
            ShowHardwareMinWarning = false;
        }



        [RelayCommand]
        public void SelectFan(FanViewModel fan)
        {
            if (SelectedFan != null)
                SelectedFan.IsSelected = false;

            SelectedFan = fan;
            if (SelectedFan != null)
            {
                SelectedFan.IsSelected = true;
                UpdateUiFromSelectedFan();
            }
        }

        private void UpdateUiFromSelectedFan()
        {
            if (SelectedFan == null) return;

            _isUpdatingUiFromSelection = true;

            try
            {
                string ctrlId = SelectedFan.ControlId;

                // Load mode
                if (!_fanModes.TryGetValue(ctrlId, out int mode))
                {
                    mode = 0; // Default: BIOS (Auto)
                }
                SelectedControlModeIndex = mode;

                // Only set the slider from polled data when NOT in manual mode.
                // In manual mode the user owns the slider value; we don't want a
                // poll cycle to reset it back to the hardware-reported duty.
                if (mode != 1)
                {
                    TargetManualSpeed = (int)SelectedFan.Duty;
                }

                // Advanced display
                AdvHardwarePath = SelectedFan.HardwarePath.Replace('_', '/');
                AdvMinRpm = SelectedFan.MinRpm;
                AdvMaxRpm = SelectedFan.MaxRpm;
                AdvControlId = SelectedFan.ControlId;
            }
            finally
            {
                _isUpdatingUiFromSelection = false;
            }
        }

        partial void OnSelectedControlModeIndexChanged(int value)
        {
            if (_isUpdatingUiFromSelection || SelectedFan == null || !SelectedFan.IsControllable || _controller == null) return;

            string ctrlId = SelectedFan.ControlId;
            _fanModes[ctrlId] = value;
            RegistrySettingsManager.SaveFanModes(_fanModes);

            if (value == 0)
            {
                // Auto - immediately release control back to BIOS
                _ = Task.Run(() => _controller.SetControlAuto(ctrlId));
                StatusBarLeftText = $"Fan control set to BIOS (Auto) for {SelectedFan.Name}.";
            }
            else if (value == 1)
            {
                // Manual - just update status; user must press Save to apply
                StatusBarLeftText = $"Manual mode selected for {SelectedFan.Name}. Set speed and press Save.";
            }
        }


        [RelayCommand]
        public async Task ApplyManualOverrideAsync()
        {
            if (SelectedFan == null || SelectedControlModeIndex != 1 || _controller == null) return;

            string ctrlId = SelectedFan.ControlId;
            int speed = TargetManualSpeed;

            var (success, actual) = await Task.Run(() => _controller.SetControlManual(ctrlId, speed));

            if (success)
            {
                await PollFansAsync();

                // Detect when the chip/BIOS clamps to a minimum — actual duty differs
                // noticeably from what was requested (>3% tolerance for rounding).
                if (Math.Abs(actual - speed) > 3f)
                {
                    ShowHardwareMinWarning = true;
                    HardwareMinWarningText = speed == 0
                        ? $"Fan could not be stopped — hardware minimum is ~{actual:F0}%. " +
                          $"To allow 0%, open BIOS → Fan Tuning and enable \"Fan Stop\" for this header."
                        : $"Requested {speed}% but hardware settled at {actual:F0}%. " +
                          $"The BIOS has a minimum fan speed set for this header.";
                    StatusBarLeftText = $"Hardware minimum active on {SelectedFan?.Name} — see banner above.";
                }
                else
                {
                    ShowHardwareMinWarning = false;
                    StatusBarLeftText = $"{speed}% applied to {SelectedFan?.Name}.";
                }
            }
            else
            {
                StatusBarLeftText = "Failed to apply manual override — check debug.log for details.";
            }
        }



        private async Task PollFansAsync()
        {
            if (_controller == null) return;

            try
            {
                // Retrieve fans directly in-process using Task.Run to keep UI responsive
                var serverFans = await Task.Run(() => _controller.GetFans());
                
                ConnectionStatusText = "Online";
                ConnectionStatusColor = "#10B981"; // green

                // Map/merge list
                var activeIds = serverFans.Select(f => f.Id).ToHashSet();

                // Remove missing fans
                var toRemove = Fans.Where(f => !activeIds.Contains(f.Id)).ToList();
                foreach (var f in toRemove) Fans.Remove(f);

                // Add or update fans
                foreach (var info in serverFans)
                {
                    var existing = Fans.FirstOrDefault(f => f.Id == info.Id);
                    if (existing == null)
                    {
                        var newFan = new FanViewModel(info);
                        Fans.Add(newFan);
                    }
                    else
                    {
                        existing.Update(info);
                    }
                }

                // If selection got lost, select first one
                if (SelectedFan != null && !Fans.Contains(SelectedFan))
                {
                    SelectedFan = null;
                }
                // Only auto-select on first load; once the user has a selection keep it.
                if (SelectedFan == null && Fans.Any())
                {
                    SelectFan(Fans[0]);
                }
                else if (SelectedFan != null)
                {
                    // Refresh advanced display info without touching the mode/slider
                    AdvHardwarePath = SelectedFan.HardwarePath.Replace('_', '/');
                    AdvMinRpm = SelectedFan.MinRpm;
                    AdvMaxRpm = SelectedFan.MaxRpm;
                    AdvControlId = SelectedFan.ControlId;
                }

                // Update detailed telemetry chart if selected
                if (SelectedFan != null)
                {
                    // Trigger UI notification
                    OnPropertyChanged(nameof(SelectedFan));
                }

                // Status Bar Right RPM list
                UpdateStatusBarRpmText();
            }
            catch (Exception ex)
            {
                ConnectionStatusText = "Error";
                ConnectionStatusColor = "#ff3b30"; // red
                StatusBarLeftText = $"Poll error: {ex.Message}";
            }
        }

        // ==========================================
        // CALIBRATION LOGIC
        // ==========================================
        [RelayCommand]
        public void StartCalibration()
        {
            if (SelectedFan == null || !SelectedFan.IsControllable || IsCalibrating || _controller == null) return;

            _calibratingControlId = SelectedFan.ControlId;
            _calibratingFanId = SelectedFan.Id;
            _calibratingFanName = SelectedFan.Name;

            // Cache current state to restore
            _fanModes.TryGetValue(_calibratingControlId, out _calibInitialMode);
            _calibInitialDuty = (int)SelectedFan.Duty;

            IsCalibrating = true;
            CalibrationShowResults = false;
            CalibrationStatus = "Initializing fan speed at 100%... (stabilizing baseline)";
            CalibrationTelemetry = "Initializing...";
            CalibrationProgress = 0;
            CalibrationPoints.Clear();

            _calibStepIndex = 0;
            _calibTicksInStep = 0;

            // Set speed to 100% manual in-process
            _ = Task.Run(() => _controller.SetControlManual(_calibratingControlId, 100));

            // Start 500ms safety timer
            _calibrationTimer.Start();
        }

        [RelayCommand]
        public void CancelCalibration()
        {
            StopCalibrationAndRestore(false, "Calibration cancelled by user.");
        }

        [RelayCommand]
        public void CloseCalibrationResults()
        {
            IsCalibrating = false;
        }

        private void StopCalibrationAndRestore(bool success, string reason = "")
        {
            _calibrationTimer.Stop();
            if (_controller == null) return;

            // Restore initial mode
            if (_calibInitialMode == 1) // Manual
            {
                _ = Task.Run(() => _controller.SetControlManual(_calibratingControlId, _calibInitialDuty));
            }
            else // Auto
            {
                _ = Task.Run(() => _controller.SetControlAuto(_calibratingControlId));
            }

            if (success)
            {
                CalibrationStatus = "Calibration Complete!";
                int minRpm = 0;
                int maxRpm = 0;
                if (CalibrationPoints.Any())
                {
                    minRpm = CalibrationPoints.Min(p => p.Speed);
                    maxRpm = CalibrationPoints.Max(p => p.Speed);
                }
                CalibrationResultsMinMax = $"Operational Speed Range: {minRpm} RPM – {maxRpm} RPM";
                CalibrationShowResults = true;
                
                StatusBarLeftText = $"Calibrated {_calibratingFanName}: {minRpm} - {maxRpm} RPM.";
            }
            else
            {
                IsCalibrating = false;
                StatusBarLeftText = string.IsNullOrEmpty(reason) ? "Calibration failed." : reason;
            }
        }

        private async Task OnCalibrationTickAsync()
        {
            if (_controller == null) return;

            try
            {
                // Fetch latest data directly in-process
                var serverFans = await Task.Run(() => _controller.GetFans());
                
                // Verify safety limits: maximum temperature of any sensor in system must not exceed 80C
                double maxTemp = 0.0;
                string hottestHw = "";
                foreach (var f in serverFans)
                {
                    if (f.TempValue > maxTemp)
                    {
                        maxTemp = f.TempValue;
                        hottestHw = f.HardwareName;
                    }
                }

                if (maxTemp >= 80.0)
                {
                    StopCalibrationAndRestore(false, $"Safety cutoff! Sensor in '{hottestHw}' reached {maxTemp:F1}°C (max allowed is 80°C).");
                    return;
                }

                // Update live telemetry display for current fan
                var targetFan = serverFans.FirstOrDefault(f => f.Id == _calibratingFanId);
                if (targetFan != null)
                {
                    _calibCurrentTemp = targetFan.TempValue;
                    _calibCurrentRpm = (int)targetFan.Rpm;

                    int currentDuty = _calibSteps[_calibStepIndex];
                    CalibrationTelemetry = $"Duty: {currentDuty}%  |  Speed: {_calibCurrentRpm} RPM  |  Temp: {_calibCurrentTemp:F1}°C";
                }

                // Advance step timer
                _calibTicksInStep++;

                // Give 100% (initial step) 6 seconds (12 ticks of 500ms) to stabilize. Other steps get 4 seconds (8 ticks)
                int ticksRequired = (_calibSteps[_calibStepIndex] == 100) ? 12 : 8;

                if (_calibTicksInStep >= ticksRequired)
                {
                    // Record point
                    int duty = _calibSteps[_calibStepIndex];
                    CalibrationPoints.Add(new RegistrySettingsManager.CurvePoint(duty, _calibCurrentRpm));

                    // Move to next step
                    _calibStepIndex++;
                    if (_calibStepIndex < _calibSteps.Count)
                    {
                        _calibTicksInStep = 0;
                        int nextDuty = _calibSteps[_calibStepIndex];
                        _ = await Task.Run(() => _controller.SetControlManual(_calibratingControlId, nextDuty));
                        CalibrationStatus = nextDuty == 100 ?
                            "Initializing speed at 100%... (stabilizing baseline)" :
                            $"Testing {nextDuty}% duty cycle... (stabilizing)";

                        CalibrationProgress = (_calibStepIndex * 100) / _calibSteps.Count;
                    }
                    else
                    {
                        // Finished
                        CalibrationProgress = 100;
                        StopCalibrationAndRestore(true);
                    }
                }
            }
            catch (Exception ex)
            {
                StopCalibrationAndRestore(false, $"Error during calibration: {ex.Message}");
            }
        }

        public void StopAll()
        {
            _pollTimer.Stop();
            _calibrationTimer.Stop();
            _controller?.Dispose();
        }
    }
}

using System;
using System.Collections.Generic;
using System.Linq;
using LibreHardwareMonitor.Hardware;

public class FanController : IDisposable
{
    private readonly Computer _computer;
    private readonly Dictionary<string, ISensor> _fanSensors = new();
    private readonly Dictionary<string, ISensor> _controlSensors = new();
    private readonly HashSet<string> _skipResetOnExitControls = new();

    public FanController()
    {
        _computer = new Computer
        {
            IsCpuEnabled = true,
            IsGpuEnabled = true,
            IsMotherboardEnabled = true,
            IsControllerEnabled = true
        };
        _computer.Open();
    }

    public void Update()
    {
        // Update all hardware to refresh sensor values
        UpdateHardware(_computer.Hardware);
    }

    private void UpdateHardware(IEnumerable<IHardware> hardwares)
    {
        foreach (var hardware in hardwares)
        {
            hardware.Update();
            foreach (var subHardware in hardware.SubHardware)
            {
                UpdateHardware(new[] { subHardware });
            }
        }
    }

    public class FanInfo
    {
        public string Id { get; set; } = "";
        public string Name { get; set; } = "";
        public float Rpm { get; set; }
        public float Min { get; set; }
        public float Max { get; set; }
        public string HardwareName { get; set; } = "";
        public string? ControlId { get; set; }
        public string Mode { get; set; } = "default";
        public float SpeedPercent { get; set; }
        public bool ResetOnExit { get; set; } = true;
    }

    public class ControlInfo
    {
        public string Id { get; set; } = "";
        public string Name { get; set; } = "";
        public string Mode { get; set; } = "default";
        public float Speed { get; set; }
        public string HardwareName { get; set; } = "";
        public bool ResetOnExit { get; set; } = true;
    }

    public List<FanInfo> GetFans()
    {
        Update();

        var fans = new List<ISensor>();
        var controls = new List<ISensor>();
        FindSensors(_computer.Hardware, fans, controls);

        // Populate our dictionaries with stable IDs
        _fanSensors.Clear();
        _controlSensors.Clear();

        var fanInfos = new List<FanInfo>();

        foreach (var fan in fans)
        {
            string fanId = SanitizeId(fan.Identifier.ToString());
            _fanSensors[fanId] = fan;

            // Try to find a control sensor in the same hardware with the same index
            var matchingControl = controls.FirstOrDefault(c =>
                c.Hardware == fan.Hardware &&
                GetSensorIndex(c.Identifier.ToString()) == GetSensorIndex(fan.Identifier.ToString()));

            string? controlId = null;
            string mode = "default";
            float speedPercent = 0;
            if (matchingControl != null)
            {
                controlId = SanitizeId(matchingControl.Identifier.ToString());
                _controlSensors[controlId] = matchingControl;
                if (matchingControl.Control != null)
                {
                    mode = matchingControl.Control.ControlMode.ToString().ToLower();
                }
                speedPercent = matchingControl.Value ?? 0;
            }

            fanInfos.Add(new FanInfo
            {
                Id = fanId,
                Name = fan.Name,
                Rpm = fan.Value ?? 0,
                Min = fan.Min ?? 0,
                Max = fan.Max ?? 0,
                HardwareName = fan.Hardware.Name,
                ControlId = controlId,
                Mode = mode,
                SpeedPercent = speedPercent,
                ResetOnExit = controlId == null || !_skipResetOnExitControls.Contains(controlId)
            });
        }

        // Also make sure any remaining controls that weren't matched are stored in _controlSensors
        foreach (var control in controls)
        {
            string ctrlId = SanitizeId(control.Identifier.ToString());
            if (!_controlSensors.ContainsKey(ctrlId))
            {
                _controlSensors[ctrlId] = control;
            }
        }

        return fanInfos;
    }

    public List<ControlInfo> GetControls()
    {
        Update();

        var fans = new List<ISensor>();
        var controls = new List<ISensor>();
        FindSensors(_computer.Hardware, fans, controls);

        var controlInfos = new List<ControlInfo>();

        foreach (var control in controls)
        {
            string ctrlId = SanitizeId(control.Identifier.ToString());
            _controlSensors[ctrlId] = control;

            string mode = "default";
            if (control.Control != null)
            {
                mode = control.Control.ControlMode.ToString().ToLower();
            }

            controlInfos.Add(new ControlInfo
            {
                Id = ctrlId,
                Name = control.Name,
                Mode = mode,
                Speed = control.Value ?? 0,
                HardwareName = control.Hardware.Name,
                ResetOnExit = !_skipResetOnExitControls.Contains(ctrlId)
            });
        }

        return controlInfos;
    }

    public bool SetControlManual(string controlId, float speedPercent, bool resetOnExit = false)
    {
        // Refresh first to populate cache
        GetControls();

        if (_controlSensors.TryGetValue(controlId, out var sensor))
        {
            if (sensor.Control != null)
            {
                sensor.Control.SetSoftware(speedPercent);
                // Always add to persistent controls list to preserve speed on exit
                _skipResetOnExitControls.Add(controlId);
                return true;
            }
        }
        return false;
    }

    public bool SetControlAuto(string controlId)
    {
        GetControls();

        if (_controlSensors.TryGetValue(controlId, out var sensor))
        {
            if (sensor.Control != null)
            {
                sensor.Control.SetDefault();
                _skipResetOnExitControls.Remove(controlId);

                // Store the manual speeds of other fans that are still in manual mode
                var otherManualControls = new Dictionary<string, float>();
                foreach (var pair in _controlSensors)
                {
                    if (pair.Key != controlId && pair.Value.Control != null && pair.Value.Control.ControlMode == ControlMode.Software)
                    {
                        otherManualControls[pair.Key] = pair.Value.Value ?? 0;
                    }
                }

                // Close the computer to completely release LPC/I/O locks and restore true BIOS auto curve settings
                _computer.Close();

                // Wait 1.5 seconds to allow motherboard BIOS SMM to reclaim control and write current temp-based speeds
                System.Threading.Thread.Sleep(1500);

                // Reopen the computer to continue monitoring
                _computer.Open();

                // Re-fetch sensors and restore manual speeds for other fans
                GetControls();
                foreach (var pair in otherManualControls)
                {
                    if (_controlSensors.TryGetValue(pair.Key, out var otherSensor))
                    {
                        otherSensor.Control?.SetSoftware(pair.Value);
                    }
                }

                return true;
            }
        }
        return false;
    }

    private void FindSensors(IEnumerable<IHardware> hardwares, List<ISensor> fans, List<ISensor> controls)
    {
        foreach (var hardware in hardwares)
        {
            foreach (var sensor in hardware.Sensors)
            {
                if (sensor.SensorType == SensorType.Fan)
                {
                    fans.Add(sensor);
                }
                else if (sensor.SensorType == SensorType.Control)
                {
                    controls.Add(sensor);
                }
            }
            FindSensors(hardware.SubHardware, fans, controls);
        }
    }

    private string SanitizeId(string identifier)
    {
        return identifier.Trim('/').Replace('/', '_');
    }

    private string GetSensorIndex(string identifier)
    {
        int slashIdx = identifier.LastIndexOf('/');
        if (slashIdx >= 0 && slashIdx < identifier.Length - 1)
        {
            return identifier.Substring(slashIdx + 1);
        }
        return "";
    }

    public void Dispose()
    {
        // Only close the computer when no controls need to persist.
        // LibreHardwareMonitor's Computer.Close() calls Control.Close() on every
        // sensor, which unconditionally calls SetDefault() on any software-controlled
        // fan — overriding the skip logic and resetting fans back to BIOS.
        // When there ARE persistent controls we skip Close(); the OS releases
        // all hardware handles when the process exits anyway.
        if (_skipResetOnExitControls.Count == 0)
        {
            _computer.Close();
        }
    }
}

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

    /// <summary>True if at least one Fan or Control sensor was found on the Motherboard hardware node.
    /// False means the Super I/O kernel driver (WinRing0) was blocked by Windows Defender / HVCI.</summary>
    public bool MotherboardSensorsDetected { get; private set; } = false;

    private void LogHardwareRecursive(string logPath, IHardware hardware, string indent)
    {
        System.IO.File.AppendAllText(logPath, $"{indent}Hardware: {hardware.Name} [{hardware.HardwareType}]\n");
        foreach (var sensor in hardware.Sensors) 
        {
            System.IO.File.AppendAllText(logPath, $"{indent}  Sensor: {sensor.Name} [{sensor.SensorType}] = {sensor.Value}\n");
        }
        foreach (var subHardware in hardware.SubHardware)
        {
            LogHardwareRecursive(logPath, subHardware, indent + "  ");
        }
    }

    public FanController()
    {
        string logDir = System.IO.Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Pankha");
        if (!System.IO.Directory.Exists(logDir))
        {
            System.IO.Directory.CreateDirectory(logDir);
        }
        string logPath = System.IO.Path.Combine(logDir, "debug.log");
        System.IO.File.WriteAllText(logPath, "Initializing FanController...\n");

        try 
        {
            _computer = new Computer
            {
                IsCpuEnabled = true,
                IsGpuEnabled = true,
                IsMotherboardEnabled = true,
                IsControllerEnabled = true
            };
            System.IO.File.AppendAllText(logPath, "Computer instance created.\n");
            
            _computer.Open();
            System.IO.File.AppendAllText(logPath, "Computer.Open() succeeded.\n");
            
            foreach (var hardware in _computer.Hardware) 
            {
                LogHardwareRecursive(logPath, hardware, "");
            }

            // Detect whether the Super I/O driver successfully loaded by checking
            // if the Motherboard hardware has any Fan or Control sub-sensors.
            foreach (var hw in _computer.Hardware)
            {
                if (hw.HardwareType == HardwareType.Motherboard)
                {
                    hw.Update();
                    foreach (var sub in hw.SubHardware)
                    {
                        sub.Update();
                        foreach (var s in sub.Sensors)
                        {
                            if (s.SensorType == SensorType.Fan || s.SensorType == SensorType.Control)
                            {
                                MotherboardSensorsDetected = true;
                                break;
                            }
                        }
                        if (MotherboardSensorsDetected) break;
                    }
                    // Also check direct sensors on the motherboard node itself
                    if (!MotherboardSensorsDetected)
                    {
                        foreach (var s in hw.Sensors)
                        {
                            if (s.SensorType == SensorType.Fan || s.SensorType == SensorType.Control)
                            {
                                MotherboardSensorsDetected = true;
                                break;
                            }
                        }
                    }
                    break;
                }
            }
            System.IO.File.AppendAllText(logPath, $"MotherboardSensorsDetected: {MotherboardSensorsDetected}\n");

            try
            {
                System.IO.File.AppendAllText(logPath, "\n--- LIBREHARDWAREMONITOR REPORT ---\n");
                System.IO.File.AppendAllText(logPath, _computer.GetReport());
                System.IO.File.AppendAllText(logPath, "\n-------------------------------------\n");
            }
            catch (Exception ex)
            {
                System.IO.File.AppendAllText(logPath, $"GetReport Exception: {ex.Message}\n");
            }
        }
        catch (Exception ex) 
        {
            System.IO.File.AppendAllText(logPath, $"Exception: {ex.ToString()}\n");
            throw;
        }
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
        public float TempValue { get; set; }
        public string TempSensorName { get; set; } = "";
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

    public class TempInfo
    {
        public string Id { get; set; } = "";
        public string Name { get; set; } = "";
        public float Value { get; set; }
        public string HardwareName { get; set; } = "";
    }

    public List<FanInfo> GetFans()
    {
        Update();

        var fans = new List<ISensor>();
        var controls = new List<ISensor>();
        var temps = new List<ISensor>();
        FindSensors(_computer.Hardware, fans, controls, temps);

        // Populate our dictionaries with stable IDs
        _fanSensors.Clear();
        _controlSensors.Clear();

        var fanInfos = new List<FanInfo>();

        foreach (var fan in fans)
        {
            string fanId = SanitizeId(fan.Identifier.ToString());
            _fanSensors[fanId] = fan;

            // Multi-strategy control-to-fan matching.
            // Strategy 1: exact same hardware object + same sensor index (works for GPU).
            string fanIndex = GetSensorIndex(fan.Identifier.ToString());
            var matchingControl = controls.FirstOrDefault(c =>
                c.Hardware == fan.Hardware &&
                GetSensorIndex(c.Identifier.ToString()) == fanIndex);

            // Strategy 2: same hardware NAME + same index.
            // LHM sometimes creates separate IHardware instances for the same physical
            // SuperIO chip, so reference equality fails for CPU/motherboard fans.
            if (matchingControl == null)
            {
                matchingControl = controls.FirstOrDefault(c =>
                    c.Hardware.Name == fan.Hardware.Name &&
                    GetSensorIndex(c.Identifier.ToString()) == fanIndex);
            }

            // Strategy 3: match purely by sensor index across all controls.
            // Needed when the fan RPM sensor lives on the CPU hardware node but the
            // control sensor lives on the motherboard's SuperIO sub-hardware node —
            // two completely different hardware paths.
            if (matchingControl == null)
            {
                matchingControl = controls.FirstOrDefault(c =>
                    GetSensorIndex(c.Identifier.ToString()) == fanIndex);
            }

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

            var primaryTemp = temps.FirstOrDefault(t => t.Hardware == fan.Hardware);
            float tempValue = 0;
            string tempSensorName = "";
            if (primaryTemp != null)
            {
                tempValue = primaryTemp.Value ?? 0;
                tempSensorName = primaryTemp.Name;
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
                ResetOnExit = controlId == null || !_skipResetOnExitControls.Contains(controlId),
                TempValue = tempValue,
                TempSensorName = tempSensorName
            });
        }

        // Controls that were never paired with a Fan RPM sensor (e.g. NCT6687D which
        // exposes only Control sensors, no Fan tachometer sensors) still need to appear
        // in the UI so the user can control them.  Create a synthetic FanInfo for each.
        foreach (var control in controls)
        {
            string ctrlId = SanitizeId(control.Identifier.ToString());

            // Register in lookup dict regardless
            if (!_controlSensors.ContainsKey(ctrlId))
                _controlSensors[ctrlId] = control;

            // Skip if already represented by a fan-based entry
            if (fanInfos.Any(f => f.ControlId == ctrlId))
                continue;

            string mode = "default";
            if (control.Control != null)
                mode = control.Control.ControlMode.ToString().ToLower();

            float duty = control.Value ?? 0;

            fanInfos.Add(new FanInfo
            {
                // Use the control ID as the fan ID so it is stable across polls
                Id           = ctrlId,
                Name         = control.Name,
                Rpm          = 0,   // no tachometer available for this channel
                Min          = 0,
                Max          = 0,
                HardwareName = control.Hardware.Name,
                ControlId    = ctrlId,
                Mode         = mode,
                SpeedPercent = duty,
                ResetOnExit  = !_skipResetOnExitControls.Contains(ctrlId),
                TempValue    = 0,
                TempSensorName = ""
            });
        }


        return fanInfos;
    }

    public List<ControlInfo> GetControls()
    {
        Update();

        var fans = new List<ISensor>();
        var controls = new List<ISensor>();
        var temps = new List<ISensor>();
        FindSensors(_computer.Hardware, fans, controls, temps);

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

    /// <returns>
    ///   success: whether the write succeeded at the LHM layer.<br/>
    ///   actualSpeed: the duty % the hardware reports after the write.
    ///   May differ from <paramref name="speedPercent"/> when the chip / BIOS
    ///   enforces a minimum fan speed (e.g. Nuvoton NCT6687D on MSI boards).
    /// </returns>
    public (bool success, float actualSpeed) SetControlManual(string controlId, float speedPercent, bool resetOnExit = false)
    {
        string logDir = System.IO.Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Pankha");
        string logPath = System.IO.Path.Combine(logDir, "debug.log");
        System.IO.File.AppendAllText(logPath, $"SetControlManual called for {controlId} with speed {speedPercent}\n");

        // Refresh first to populate cache
        GetControls();

        if (_controlSensors.TryGetValue(controlId, out var sensor))
        {
            System.IO.File.AppendAllText(logPath, $"Found control sensor {sensor.Name} with ID {controlId}.\n");
            if (sensor.Control != null)
            {
                try
                {
                    sensor.Control.SetSoftware(speedPercent);

                    // Read back the actual duty the hardware settled on.
                    // Some chips (e.g. NCT6687D on MSI boards) clamp the PWM to a
                    // BIOS-enforced minimum even in software-control mode, so the
                    // reported value may be higher than what was requested.
                    System.Threading.Thread.Sleep(120);
                    sensor.Hardware.Update();
                    float actual = sensor.Value ?? speedPercent;

                    System.IO.File.AppendAllText(logPath,
                        $"Set {sensor.Name} to {speedPercent}% — hardware reports {actual:F1}%.\n");

                    _skipResetOnExitControls.Add(controlId);
                    return (true, actual);
                }
                catch (Exception ex)
                {
                    System.IO.File.AppendAllText(logPath, $"Exception setting software control: {ex.Message}\n");
                }
            }
            else
            {
                System.IO.File.AppendAllText(logPath, $"Sensor {sensor.Name} Control property is null!\n");
            }
        }
        else
        {
            System.IO.File.AppendAllText(logPath, $"Control sensor {controlId} not found in _controlSensors!\n");
        }
        return (false, speedPercent);
    }

    public bool SetControlAuto(string controlId)
    {
        string logDir = System.IO.Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Pankha");
        string logPath = System.IO.Path.Combine(logDir, "debug.log");
        System.IO.File.AppendAllText(logPath, $"SetControlAuto called for {controlId}\n");

        GetControls();

        if (_controlSensors.TryGetValue(controlId, out var sensor))
        {
            System.IO.File.AppendAllText(logPath, $"Found control sensor {sensor.Name} with ID {controlId} for Auto.\n");
            if (sensor.Control != null)
            {
                try
                {
                    sensor.Control.SetDefault();
                    System.IO.File.AppendAllText(logPath, $"Successfully set {sensor.Name} to auto BIOS mode.\n");
                    _skipResetOnExitControls.Remove(controlId);
                    return true;
                }
                catch (Exception ex)
                {
                    System.IO.File.AppendAllText(logPath, $"Exception setting auto control: {ex.Message}\n");
                }
            }
            else
            {
                System.IO.File.AppendAllText(logPath, $"Sensor {sensor.Name} Control property is null!\n");
            }
        }
        else
        {
            System.IO.File.AppendAllText(logPath, $"Control sensor {controlId} not found in _controlSensors!\n");
        }
        return false;
    }

    public List<TempInfo> GetTemperatures()
    {
        Update();

        var fans = new List<ISensor>();
        var controls = new List<ISensor>();
        var temps = new List<ISensor>();
        FindSensors(_computer.Hardware, fans, controls, temps);

        var tempInfos = new List<TempInfo>();
        foreach (var temp in temps)
        {
            tempInfos.Add(new TempInfo
            {
                Id = SanitizeId(temp.Identifier.ToString()),
                Name = temp.Name,
                Value = temp.Value ?? 0,
                HardwareName = temp.Hardware.Name
            });
        }
        return tempInfos;
    }

    private void FindSensors(IEnumerable<IHardware> hardwares, List<ISensor> fans, List<ISensor> controls, List<ISensor> temperatures)
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
                else if (sensor.SensorType == SensorType.Temperature)
                {
                    temperatures.Add(sensor);
                }
            }
            FindSensors(hardware.SubHardware, fans, controls, temperatures);
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

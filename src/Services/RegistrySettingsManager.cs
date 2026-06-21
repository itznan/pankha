using System;
using System.Collections.Generic;
using System.IO;
using Microsoft.Win32;

namespace frontend_avalonia.Services
{
    [System.Runtime.Versioning.SupportedOSPlatform("windows")]
    public class RegistrySettingsManager
    {
        private const string RegistryKeyPath = @"Software\itznan\Pankha";
        private const string RunRegistryKeyPath = @"Software\Microsoft\Windows\CurrentVersion\Run";

        public struct CurvePoint
        {
            public int Temp { get; set; }
            public int Speed { get; set; }
            public CurvePoint(int t, int s) { Temp = t; Speed = s; }
        }

        public class FanCurveSettings
        {
            public string ControlId { get; set; } = string.Empty;
            public List<CurvePoint> Points { get; set; } = new();
        }

        public static int LoadInt(string name, int defaultValue)
        {
            try
            {
                using var key = Registry.CurrentUser.OpenSubKey(RegistryKeyPath);
                if (key != null)
                {
                    var val = key.GetValue(name);
                    if (val != null) return Convert.ToInt32(val);
                }
            }
            catch { }
            return defaultValue;
        }

        public static bool LoadBool(string name, bool defaultValue)
        {
            try
            {
                using var key = Registry.CurrentUser.OpenSubKey(RegistryKeyPath);
                if (key != null)
                {
                    var val = key.GetValue(name);
                    if (val != null) return Convert.ToBoolean(Convert.ToInt32(val));
                }
            }
            catch { }
            return defaultValue;
        }

        public static void SaveValue(string name, object value)
        {
            try
            {
                using var key = Registry.CurrentUser.CreateSubKey(RegistryKeyPath);
                if (key != null)
                {
                    if (value is bool b)
                    {
                        key.SetValue(name, b ? 1 : 0, RegistryValueKind.DWord);
                    }
                    else if (value is int i)
                    {
                        key.SetValue(name, i, RegistryValueKind.DWord);
                    }
                    else
                    {
                        key.SetValue(name, value.ToString() ?? "");
                    }
                }
            }
            catch { }
        }

        public static void SetStartOnBoot(bool enabled)
        {
            try
            {
                using var key = Registry.CurrentUser.OpenSubKey(RunRegistryKeyPath, true);
                if (key != null)
                {
                    if (enabled)
                    {
                        string appPath = ProcessPathHelper.GetExecutablePath();
                        key.SetValue("PankhaFanControl", $"\"{appPath}\" --startup");
                    }
                    else
                    {
                        key.DeleteValue("PankhaFanControl", false);
                    }
                }
            }
            catch { }
        }

        public static Dictionary<string, int> LoadFanModes()
        {
            var modes = new Dictionary<string, int>();
            try
            {
                using var key = Registry.CurrentUser.OpenSubKey(RegistryKeyPath);
                if (key != null)
                {
                    var countVal = key.GetValue("fanModes_count");
                    if (countVal != null)
                    {
                        int count = Convert.ToInt32(countVal);
                        for (int i = 0; i < count; i++)
                        {
                            string id = key.GetValue($"fanModes_{i}_id")?.ToString() ?? "";
                            var modeVal = key.GetValue($"fanModes_{i}_mode");
                            if (!string.IsNullOrEmpty(id) && modeVal != null)
                            {
                                modes[id] = Convert.ToInt32(modeVal);
                            }
                        }
                    }
                }
            }
            catch { }
            return modes;
        }

        public static void SaveFanModes(Dictionary<string, int> modes)
        {
            try
            {
                using var key = Registry.CurrentUser.CreateSubKey(RegistryKeyPath);
                if (key != null)
                {
                    key.SetValue("fanModes_count", modes.Count, RegistryValueKind.DWord);
                    int i = 0;
                    foreach (var pair in modes)
                    {
                        key.SetValue($"fanModes_{i}_id", pair.Key);
                        key.SetValue($"fanModes_{i}_mode", pair.Value, RegistryValueKind.DWord);
                        i++;
                    }
                }
            }
            catch { }
        }

        public static Dictionary<string, List<CurvePoint>> LoadFanCurves()
        {
            var curves = new Dictionary<string, List<CurvePoint>>();
            try
            {
                using var key = Registry.CurrentUser.OpenSubKey(RegistryKeyPath);
                if (key != null)
                {
                    var countVal = key.GetValue("fanCurves_count");
                    if (countVal != null)
                    {
                        int count = Convert.ToInt32(countVal);
                        for (int i = 0; i < count; i++)
                        {
                            string id = key.GetValue($"fanCurves_{i}_id")?.ToString() ?? "";
                            if (!string.IsNullOrEmpty(id))
                            {
                                var list = new List<CurvePoint>();
                                for (int pt = 1; pt <= 4; pt++)
                                {
                                    int t = Convert.ToInt32(key.GetValue($"fanCurves_{i}_t{pt}", 0));
                                    int s = Convert.ToInt32(key.GetValue($"fanCurves_{i}_s{pt}", 0));
                                    list.Add(new CurvePoint(t, s));
                                }
                                curves[id] = list;
                            }
                        }
                    }
                }
            }
            catch { }
            return curves;
        }

        public static void SaveFanCurves(Dictionary<string, List<CurvePoint>> curves)
        {
            try
            {
                using var key = Registry.CurrentUser.CreateSubKey(RegistryKeyPath);
                if (key != null)
                {
                    key.SetValue("fanCurves_count", curves.Count, RegistryValueKind.DWord);
                    int i = 0;
                    foreach (var pair in curves)
                    {
                        key.SetValue($"fanCurves_{i}_id", pair.Key);
                        for (int pt = 1; pt <= 4; pt++)
                        {
                            var point = pair.Value[pt - 1];
                            key.SetValue($"fanCurves_{i}_t{pt}", point.Temp, RegistryValueKind.DWord);
                            key.SetValue($"fanCurves_{i}_s{pt}", point.Speed, RegistryValueKind.DWord);
                        }
                        i++;
                    }
                }
            }
            catch { }
        }
    }

    public static class ProcessPathHelper
    {
        public static string GetExecutablePath()
        {
            return Environment.ProcessPath ?? System.Diagnostics.Process.GetCurrentProcess().MainModule?.FileName ?? "";
        }
    }
}

using System;
using System.Diagnostics;
using System.IO;
using System.Net.Http;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Win32;

namespace frontend_avalonia.Services
{
    public static class PawnIOChecker
    {
        // Official PawnIO installer (namazso/PawnIO.Setup on GitHub)
        private const string DownloadUrl =
            "https://github.com/namazso/PawnIO.Setup/releases/download/2.2.0/PawnIO.Setup.exe";

        private const string InstallerFileName = "PawnIO.Setup.exe";

        /// <summary>
        /// Returns true if the PawnIO kernel driver service is already registered in the Windows registry.
        /// </summary>
        public static bool IsInstalled()
        {
            try
            {
                using var key = Registry.LocalMachine.OpenSubKey(
                    @"SYSTEM\CurrentControlSet\Services\PawnIO", writable: false);
                return key != null;
            }
            catch
            {
                return false;
            }
        }

        /// <summary>
        /// Downloads the PawnIO installer to a temp path, reporting download progress via <paramref name="progress"/>.
        /// Returns the path to the downloaded file.
        /// </summary>
        public static async Task<string> DownloadInstallerAsync(
            IProgress<(int percent, string status)> progress,
            CancellationToken ct = default)
        {
            string tempPath = Path.Combine(Path.GetTempPath(), InstallerFileName);

            progress.Report((0, "Connecting to GitHub…"));

            using var client = new HttpClient();
            client.DefaultRequestHeaders.Add("User-Agent", "Pankha-FanControl");

            using var response = await client.GetAsync(DownloadUrl, HttpCompletionOption.ResponseHeadersRead, ct);
            response.EnsureSuccessStatusCode();

            long total = response.Content.Headers.ContentLength ?? -1;
            long downloaded = 0;

            await using var stream = await response.Content.ReadAsStreamAsync(ct);
            await using var file   = File.Create(tempPath);

            var buffer = new byte[81920];
            int read;
            while ((read = await stream.ReadAsync(buffer, ct)) > 0)
            {
                await file.WriteAsync(buffer.AsMemory(0, read), ct);
                downloaded += read;

                int pct = total > 0 ? (int)(downloaded * 100 / total) : 0;
                progress.Report((pct, $"Downloading PawnIO… {downloaded / 1024:N0} KB" +
                                      (total > 0 ? $" / {total / 1024:N0} KB" : "")));
            }

            progress.Report((100, "Download complete."));
            return tempPath;
        }

        /// <summary>
        /// Runs the PawnIO installer silently (requires UAC elevation – app already runs as admin).
        /// Returns true if the installer succeeded (exit code 0 = OK, 3010 = reboot required).
        /// <paramref name="rebootRequired"/> is set true when the installer reports exit 3010.
        /// </summary>
        public static async Task<(bool success, bool rebootRequired)> RunInstallerAsync(
            string installerPath,
            IProgress<(int percent, string status)> progress)
        {
            progress.Report((100, "Running PawnIO installer…"));

            var psi = new ProcessStartInfo
            {
                FileName  = installerPath,
                Arguments = "/silent",   // Inno Setup silent flag
                UseShellExecute  = true, // needed for UAC
                Verb             = "runas",
                CreateNoWindow   = true,
            };

            using var proc = Process.Start(psi);
            if (proc == null)
                return (false, false);

            await proc.WaitForExitAsync();

            // Inno Setup: 0 = success, 3010 = success + reboot required
            bool reboot  = proc.ExitCode == 3010;
            bool success = proc.ExitCode == 0 || reboot;

            progress.Report((100, success
                ? (reboot ? "Installed. A restart is recommended." : "PawnIO installed successfully!")
                : $"Installer failed (exit {proc.ExitCode})."));

            return (success, reboot);
        }

        /// <summary>
        /// Convenience: download then install, reporting all progress.
        /// </summary>
        public static async Task<(bool success, bool rebootRequired)> DownloadAndInstallAsync(
            IProgress<(int percent, string status)> progress,
            CancellationToken ct = default)
        {
            string path = await DownloadInstallerAsync(progress, ct);
            return await RunInstallerAsync(path, progress);
        }
    }
}

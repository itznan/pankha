using System;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Threading;
using frontend_avalonia.Services;

namespace frontend_avalonia.Views
{
    public partial class PawnIOInstallWindow : Window
    {
        /// <summary>True if the user clicked "Skip for now".</summary>
        public bool Skipped { get; private set; } = false;

        /// <summary>True when installation completed and a system reboot is recommended.</summary>
        public bool RebootRequired { get; private set; } = false;

        /// <summary>True when PawnIO was successfully installed during this session.</summary>
        public bool InstallSucceeded { get; private set; } = false;

        private bool _isInstalling = false;

        public PawnIOInstallWindow()
        {
            InitializeComponent();
        }

        private void InitializeComponent()
        {
            Avalonia.Markup.Xaml.AvaloniaXamlLoader.Load(this);
        }

        private void OnSkip(object? sender, RoutedEventArgs e)
        {
            if (_isInstalling) return;
            Skipped = true;
            Close();
        }

        private async void OnInstall(object? sender, RoutedEventArgs e)
        {
            if (_isInstalling) return;
            _isInstalling = true;

            var installBtn  = this.FindControl<Button>("InstallButton")!;
            var skipBtn     = this.FindControl<Button>("SkipButton")!;
            var progressBar = this.FindControl<ProgressBar>("InstallProgress")!;
            var statusLabel = this.FindControl<TextBlock>("StatusLabel")!;
            var progressPanel = this.FindControl<StackPanel>("ProgressPanel")!;

            installBtn.IsEnabled = false;
            skipBtn.IsEnabled    = false;
            progressPanel.IsVisible = true;
            installBtn.Content   = "Installing…";

            var progress = new Progress<(int percent, string status)>(report =>
            {
                Dispatcher.UIThread.Post(() =>
                {
                    progressBar.Value = report.percent;
                    statusLabel.Text  = report.status;
                });
            });

            try
            {
                var (success, rebootRequired) = await PawnIOChecker.DownloadAndInstallAsync(progress);

                InstallSucceeded = success;
                RebootRequired   = rebootRequired;

                if (success)
                {
                    installBtn.Content = rebootRequired ? "Restart recommended ✓" : "Installed ✓";
                    installBtn.Classes.Remove("primary");
                    installBtn.Classes.Add("secondary");

                    skipBtn.Content   = "Continue";
                    skipBtn.IsEnabled = true;
                }
                else
                {
                    statusLabel.Text     = "Installation failed. You can try installing PawnIO manually from github.com/namazso/PawnIO.Setup";
                    installBtn.Content   = "Retry";
                    installBtn.IsEnabled = true;
                    skipBtn.IsEnabled    = true;
                    _isInstalling        = false;
                    return;
                }
            }
            catch (Exception ex)
            {
                statusLabel.Text     = $"Error: {ex.Message}";
                installBtn.Content   = "Retry";
                installBtn.IsEnabled = true;
                skipBtn.IsEnabled    = true;
                _isInstalling        = false;
                return;
            }

            // After success wait a moment then auto-close
            await Task.Delay(1200);
            Close();
        }
    }
}

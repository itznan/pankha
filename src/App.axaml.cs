using System;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Markup.Xaml;
using Avalonia.Threading;
using frontend_avalonia.Services;
using frontend_avalonia.ViewModels;
using frontend_avalonia.Views;

namespace frontend_avalonia
{
    public partial class App : Application
    {
        public override void Initialize()
        {
            AvaloniaXamlLoader.Load(this);
        }

        public override async void OnFrameworkInitializationCompleted()
        {
            if (ApplicationLifetime is IClassicDesktopStyleApplicationLifetime desktop)
            {
                // ── PawnIO prerequisite check ────────────────────────────────────────
                // Run the check on the UI thread so we can show the dialog.
                if (!PawnIOChecker.IsInstalled())
                {
                    var installWindow = new PawnIOInstallWindow();

                    // Show the install dialog modally before creating the main window.
                    // We use a TaskCompletionSource because ShowDialog needs a parent
                    // window but we don't have one yet; instead we show it as the
                    // temporary startup window and wait for it to close.
                    desktop.MainWindow = installWindow;
                    
                    // Wait for the install window to close
                    var tcs = new TaskCompletionSource<bool>();
                    installWindow.Closed += (_, _) => tcs.SetResult(true);

                    // Start the app (shows the install window)
                    base.OnFrameworkInitializationCompleted();

                    await tcs.Task;

                    // If a reboot was flagged, let the user know and exit
                    if (installWindow.RebootRequired)
                    {
                        var rebootNotice = new Window
                        {
                            Title  = "Restart Required – Pankha",
                            Width  = 380,
                            Height = 160,
                            WindowStartupLocation = WindowStartupLocation.CenterScreen,
                            CanResize = false,
                            Background = Avalonia.Media.Brush.Parse("#1c1c1e"),
                            Content = new Avalonia.Controls.TextBlock
                            {
                                Text = "PawnIO was installed successfully.\n\nPlease restart your PC, then launch Pankha again.",
                                Foreground = Avalonia.Media.Brush.Parse("#f5f5f7"),
                                TextWrapping = Avalonia.Media.TextWrapping.Wrap,
                                FontSize = 14,
                                Margin = new Thickness(28),
                                VerticalAlignment = Avalonia.Layout.VerticalAlignment.Center,
                            }
                        };
                        desktop.MainWindow = rebootNotice;
                        rebootNotice.Show();
                        return;
                    }

                    // Swap to the real main window
                    var mainWindow = new MainWindow
                    {
                        DataContext = new MainWindowViewModel(),
                    };
                    desktop.MainWindow = mainWindow;
                    mainWindow.Show();
                    return;
                }

                // ── Normal startup (PawnIO already installed) ────────────────────────
                desktop.MainWindow = new MainWindow
                {
                    DataContext = new MainWindowViewModel(),
                };
            }

            base.OnFrameworkInitializationCompleted();
        }
    }
}
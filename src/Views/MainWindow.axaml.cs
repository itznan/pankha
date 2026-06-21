using System;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Avalonia.Media;
using Avalonia.Styling;
using frontend_avalonia.ViewModels;

namespace frontend_avalonia.Views
{
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
        }

        private void InitializeComponent()
        {
            AvaloniaXamlLoader.Load(this);
        }

        protected override void OnOpened(EventArgs e)
        {
            base.OnOpened(e);
            
            // Initial theme styling setup
            UpdateThemeBrushes();
        }

        private void UpdateThemeBrushes()
        {
            var mainBorder = this.FindControl<Border>("MainBorder");
            var sidebarBorder = this.FindControl<Border>("SidebarBorder");

            if (mainBorder == null || sidebarBorder == null) return;

            mainBorder.Background = new SolidColorBrush(Color.Parse("#1E1E1F"), 0.9);
            sidebarBorder.Background = new SolidColorBrush(Color.Parse("#181819"), 0.8);
        }



        private void FanCard_PointerPressed(object? sender, PointerPressedEventArgs e)
        {
            if (sender is Border border && border.Tag is FanViewModel fan)
            {
                if (DataContext is MainWindowViewModel vm)
                {
                    vm.SelectFan(fan);
                }
            }
        }



        protected override void OnClosing(WindowClosingEventArgs e)
        {
            var vm = DataContext as MainWindowViewModel;
            vm?.StopAll();
            base.OnClosing(e);
        }
    }
}
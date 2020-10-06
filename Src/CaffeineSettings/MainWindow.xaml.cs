using System;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Interop;

namespace CaffeineSettings
{
    public static class Kernel32Interop
    {
        public const uint STANDARD_RIGHTS_REQUIRED = 0x000F0000;
        public const uint SYNCHRONIZE = 0x00100000;
        public const uint EVENT_ALL_ACCESS = (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x3);
        public const uint EVENT_MODIFY_STATE = 0x0002;
        public const long ERROR_FILE_NOT_FOUND = 2L;

        [DllImport("Kernel32.dll", SetLastError = true)]
        public static extern IntPtr OpenEvent(uint dwDesiredAccess, bool bInheritHandle, string lpName);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool CloseHandle(IntPtr hObject);

        [DllImport("kernel32.dll")]
        public static extern bool SetEvent(IntPtr hEvent);
    }


    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private const string settingsFilename = "Caffeine.json";
        private const string reloadEventName  = "CaffeineTray_ReloadEvent";
        
        public MainWindow()
        {
            InitializeComponent();

            try
            {
                var settings = Settings.Load(settingsFilename);
                DataContext = new SettingsViewModel(settings);
            }
            catch
            {
                MessageBox.Show("Failed to load settings file '" + settingsFilename + "'");
                DataContext = new SettingsViewModel(new Settings());
            }
        }

        private void ListView_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            var listView = sender as ListView;
            var gridView = listView.View as GridView;
            
            // take into account vertical scrollbar
            var workingWidth = listView.ActualWidth - SystemParameters.VerticalScrollBarWidth;
            var otherColumnsWidth = gridView.Columns[0].Width + gridView.Columns[2].Width;

            gridView.Columns[1].Width = workingWidth - otherColumnsWidth;
        }

        private void BtnAdd_Click(object sender, RoutedEventArgs e)
        {
            var addProcessWnd = new AddDialog(DataContext as SettingsViewModel);
            addProcessWnd.ShowDialog();
        }

        private void BtnRemove_Click(object sender, RoutedEventArgs e)
        {
            var dc = DataContext as SettingsViewModel;
            var selected = lvProcessWindow.SelectedItem as ProcessWindowItem;
            dc.Remove(selected);
        }

        private void BtnApply_Click(object sender, RoutedEventArgs e)
        {
            var dc = DataContext as SettingsViewModel;
            var settings = dc.ToSettingsJson();

            if (!Settings.Save(settingsFilename, settings))
            {
                MessageBox.Show("Failed to save settings file '" + settingsFilename + "'");
            }

            // Trigger settings reload.
            var reloadEvent = Kernel32Interop.OpenEvent(Kernel32Interop.EVENT_ALL_ACCESS, false, reloadEventName);
            if (reloadEvent != IntPtr.Zero)
            {
                Kernel32Interop.SetEvent(reloadEvent);
                Kernel32Interop.CloseHandle(reloadEvent);
            }

            Application.Current.Shutdown();
        }

        private void BtnCancel_Click(object sender, RoutedEventArgs e)
        {
            Application.Current.Shutdown();
        }
    }
}

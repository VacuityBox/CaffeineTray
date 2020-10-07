using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media.Imaging;

namespace CaffeineSettings
{
    internal static class Extensions
    {
        [DllImport("Kernel32.dll")]
        private static extern bool QueryFullProcessImageName([In] IntPtr hProcess, [In] uint dwFlags, [Out] StringBuilder lpExeName, [In, Out] ref uint lpdwSize);

        public static string GetMainModuleFileName(this Process process, int buffer = 1024)
        {
            var fileNameBuilder = new StringBuilder(buffer);
            uint bufferLength = (uint)fileNameBuilder.Capacity + 1;

            try
            {
                return QueryFullProcessImageName(process.Handle, 0, fileNameBuilder, ref bufferLength) ?
                    fileNameBuilder.ToString() :
                    null;
            }
            catch
            {
                return null;
            }
        }
    }

    public class RunningProcess
    {
        public BitmapImage Icon { get; set; }
        public string Name { get; set; }
        public string Path { get; set; }
        public string Window { get; set; }

        public RunningProcess(BitmapImage icon, string name, string path, string window)
        {
            Icon = icon;
            Name = name;
            Path = path;
            Window = window;
        }

        public static List<RunningProcess> GetProcessList()
        {
            // This is slow on debug.
            var processList = Process.GetProcesses();
            var runningProcesses = new Dictionary<string, RunningProcess>();

            foreach (var p in processList)
            {
                var path = p.GetMainModuleFileName();
                var name = path is null || path.Length < 1 ? p.ProcessName : System.IO.Path.GetFileName(path);
                var icon = IconCache.GetIcon(path);
                var title = p.MainWindowTitle;
                try
                {
                    runningProcesses.Add(
                        path is null ? name : path,
                        new RunningProcess(icon, name, path, title)
                    );
                }
                catch
                {
                }
            }

            return runningProcesses.Values.ToList();
        }
    }

    public class RunningProcessViewModel
    {
        public ObservableCollection<RunningProcess> RunningProcesses { get; set; }

        public RunningProcessViewModel()
        {
            RunningProcesses = new ObservableCollection<RunningProcess>();
        }

        public async void Update()
        {
            var procList = await Task.Run(() => RunningProcess.GetProcessList());
            foreach (var proc in procList)
                RunningProcesses.Add(proc);
        }
    }

    /// <summary>
    /// Interaction logic for AddDialog.xaml
    /// </summary>
    public partial class AddDialog : Window
    {
        private SettingsViewModel SettingsViewModel;

        public AddDialog(SettingsViewModel svm)
        {
            InitializeComponent();

            SettingsViewModel = svm;
            var rpvm = new RunningProcessViewModel();
            DataContext = rpvm;

            rpvm.Update();
        }

        private void BtnOpenProcess_Click(object sender, RoutedEventArgs e)
        {
            var dlg = new Microsoft.Win32.OpenFileDialog();
            dlg.DefaultExt = ".exe";
            dlg.Filter = "Applications (*.exe)|*.exe";

            var result = dlg.ShowDialog();
            if (result == true)
            {
                var path = dlg.FileName;
                tbProcessPath.Text = path;
                tbProcessName.Text = Path.GetFileName(path);
                tbWindowTitle.Text = "";
            }
        }

        private void BtnAddName_Click(object sender, RoutedEventArgs e)
        {
            SettingsViewModel.AddName(tbProcessName.Text);
            this.Close();
        }

        private void BtnAddPath_Click(object sender, RoutedEventArgs e)
        {
            // TODO validate path? or dont care 
            SettingsViewModel.AddPath(tbProcessPath.Text);
            this.Close();
        }

        private void BtnAddWindow_Click(object sender, RoutedEventArgs e)
        {
            SettingsViewModel.AddWindow(tbWindowTitle.Text);
            this.Close();
        }

        private void lvRunningProcessWindow_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var lv = sender as ListView;
            var selected = lv.SelectedItem as RunningProcess;

            tbProcessName.Text = selected.Name;
            tbProcessPath.Text = selected.Path;
            tbWindowTitle.Text = selected.Window;
        }

        private void lvRunningProcessWindow_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            var listView = sender as ListView;
            var gridView = listView.View as GridView;

            // take into account vertical scrollbar
            var workingWidth = listView.ActualWidth - SystemParameters.VerticalScrollBarWidth;
            var otherColumnsWidth = gridView.Columns[0].ActualWidth;

            gridView.Columns[1].Width = workingWidth - otherColumnsWidth - 10;
        }
    }
}

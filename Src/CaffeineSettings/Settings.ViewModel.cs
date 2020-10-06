using System;
using System.Collections.ObjectModel;
using System.Globalization;
using System.Windows.Data;
using System.Windows.Media.Imaging;

namespace CaffeineSettings
{
    public enum ProcessWindowType
    {
        Path,
        Name,
        Window
    }

    public class ProcessWindowItem
    {
        public BitmapImage Icon { get; set; }
        public string Name { get; set; }
        public ProcessWindowType Type { get; set; }

        public ProcessWindowItem(string name, ProcessWindowType type)
        {
            if (type == ProcessWindowType.Path)
                this.Icon = IconCache.GetIcon(name);
            else
                this.Icon = new BitmapImage();

            this.Name = name;
            this.Type = type;
        }
    }

    [ValueConversion(typeof(ProcessWindowType), typeof(string))]
    public class ProcessWindowTypeConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value is ProcessWindowType && value != null)
            {
                switch (value)
                {
                    case ProcessWindowType.Path: return "Process path";
                    case ProcessWindowType.Name: return "Process name";
                    case ProcessWindowType.Window: return "Window title";
                }
            }

            return "";
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class StandardViewModel
    {
        public bool keepDisplayOn { get; set; }
    }

    public class AutoViewModel
    {
        public bool keepDisplayOn { get; set; }
        public int scanInterval { get; set; }
        public ObservableCollection<ProcessWindowItem> ProcessWindowList { get; set; }

        public AutoViewModel(AutoMode auto)
        {
            keepDisplayOn = auto.keepDisplayOn;
            scanInterval = auto.scanInterval;

            ProcessWindowList = new ObservableCollection<ProcessWindowItem>();
            foreach (var path in auto.processPaths)
            {
                ProcessWindowList.Add(new ProcessWindowItem(path, ProcessWindowType.Path));
            }
            foreach (var name in auto.processNames)
            {
                ProcessWindowList.Add(new ProcessWindowItem(name, ProcessWindowType.Name));
            }
            foreach (var title in auto.windowTitles)
            {
                ProcessWindowList.Add(new ProcessWindowItem(title, ProcessWindowType.Window));
            }
        }
    }

    public class SettingsViewModel
    {
        public Mode Mode { get; set; }
        public StandardViewModel Standard { get; set; }
        public AutoViewModel Auto { get; set; }

        public SettingsViewModel(Settings settings)
        {
            Mode = settings.Mode;

            Standard = new StandardViewModel();
            Standard.keepDisplayOn = settings.Standard.keepDisplayOn;

            Auto = new AutoViewModel(settings.Auto);
        }

        public bool AddName(string name)
        {
            if (name is null || name.Length < 1)
                return false;

            // Check if not already on list.
            foreach (var item in Auto.ProcessWindowList)
            {
                if (item.Name == name)
                    return false;
            }

            Auto.ProcessWindowList.Add(new ProcessWindowItem(name, ProcessWindowType.Name));

            return true;
        }

        public bool AddPath(string path)
        {
            if (path is null || path.Length < 1)
                return false;

            // Check if not already on list.
            foreach (var item in Auto.ProcessWindowList)
            {
                if (item.Name == path)
                    return false;
            }

            Auto.ProcessWindowList.Add(new ProcessWindowItem(path, ProcessWindowType.Path));

            return true;
        }

        public bool AddWindow(string title)
        {
            if (title is null || title.Length < 1)
                return false;

            // Check if not already on list.
            foreach (var item in Auto.ProcessWindowList)
            {
                if (item.Name == title)
                    return false;
            }

            Auto.ProcessWindowList.Add(new ProcessWindowItem(title, ProcessWindowType.Window));

            return true;
        }

        public bool Remove(ProcessWindowItem item)
        {
            foreach (var pw in Auto.ProcessWindowList)
            {
                if (pw.Type == item.Type)
                {
                    if (pw.Name == item.Name)
                    {
                        Auto.ProcessWindowList.Remove(item);
                        return true;
                    }
                }
            }

            return false;
        }

        public Settings ToSettingsJson()
        {
            var json = new Settings();

            {
                json.Mode = Mode;
            }

            // Standard
            {
                json.Standard.keepDisplayOn = Standard.keepDisplayOn;
            }

            // Auto
            {
                json.Auto.keepDisplayOn = Auto.keepDisplayOn;
                json.Auto.scanInterval = Auto.scanInterval;
                foreach (var p in Auto.ProcessWindowList)
                {
                    switch (p.Type)
                    {
                        case ProcessWindowType.Path:
                            json.Auto.processPaths.Add(p.Name);
                            break;
                        case ProcessWindowType.Name:
                            json.Auto.processNames.Add(p.Name);
                            break;
                        case ProcessWindowType.Window:
                            json.Auto.windowTitles.Add(p.Name);
                            break;
                    }
                }
            }

            return json;
        }
    }
}

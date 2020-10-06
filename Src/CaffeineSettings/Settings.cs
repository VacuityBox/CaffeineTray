using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Globalization;
using System.IO;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Json;
using System.Text;
using System.Text.Json;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Xml;

namespace CaffeineSettings
{
    public enum Mode
    {
        Disabled,
        Enabled,
        Auto_Inactive,
        Auto_Active
    }

    public class StandardMode
    {
        public bool keepDisplayOn { get; set; } = false;
    }

    public class AutoMode
    {
        public bool keepDisplayOn { get; set; } = false;
        public int  scanInterval  { get; set; } = 2000;

        public List<string> processPaths { get; set; } = new List<string>();
        public List<string> processNames { get; set; } = new List<string>();
        public List<string> windowTitles { get; set; } = new List<string>();
    }

    [DataContract(Name = "Settings", Namespace = "")]
    public class Settings
    {
        #region Json

        [DataMember(Name = "mode", Order = 0)]
        public Mode Mode { get; set; } = Mode.Disabled;

        [DataMember(Name = "Standard", Order = 1)]
        public StandardMode Standard { get; set; } = new StandardMode();

        [DataMember(Name = "Auto", Order = 2)]
        public AutoMode Auto { get; set; } = new AutoMode();

        #endregion Json

        #region Methods

        static public Settings Load(string filename)
        {
            if (File.Exists(filename))
            {
                using (var fileStream = new FileStream(filename, FileMode.Open))
                {
                    var serializer = new DataContractJsonSerializer(typeof(Settings));
                    using (var reader = JsonReaderWriterFactory.CreateJsonReader(
                        fileStream,
                        Encoding.UTF8,
                        XmlDictionaryReaderQuotas.Max,
                        dictionaryReader => { }))
                    {
                        try
                        {
                            return (Settings)serializer.ReadObject(reader);
                        }
                        catch
                        {
                            throw;
                        }
                    }
                }
            }

            return new Settings();
        }

        static public bool Save(string filename, Settings settings)
        {
            using (var fileStream = new FileStream(filename, FileMode.Create))
            {
                var serializer = new DataContractJsonSerializer(typeof(Settings));
                using (var writer = JsonReaderWriterFactory.CreateJsonWriter(
                    fileStream,
                    Encoding.UTF8,
                    true,
                    true))
                {
                    serializer.WriteObject(writer, settings);
                }
            }

            return true;
        }

        #endregion Methods
    }
}

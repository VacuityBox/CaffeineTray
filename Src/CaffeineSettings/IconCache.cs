using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Windows.Media.Imaging;

namespace CaffeineSettings
{
    public static class IconCache
    {
        private static Dictionary<string, BitmapImage> icons { get; set; } = null;

        public static BitmapImage GetIcon(string executablePath)
        {
            if (executablePath is null)
                return null;

            if (icons == null)
                icons = new Dictionary<string, BitmapImage>();

            // Try to get icon from cache.
            var bmp = new BitmapImage();
            if (!icons.TryGetValue(executablePath, out bmp))
            {
                // Load new icon.
                try
                {
                    var exeIcon = Icon.ExtractAssociatedIcon(executablePath);
                    bmp = ToBitmapImage(exeIcon.ToBitmap());
                    icons.Add(executablePath, bmp);
                }
                catch
                {
                }
            }

            return bmp;
        }

        private static BitmapImage ToBitmapImage(Bitmap bitmap)
        {
            using (var memory = new MemoryStream())
            {
                bitmap.Save(memory, ImageFormat.Png);
                memory.Position = 0;

                var bitmapImage = new BitmapImage();
                bitmapImage.BeginInit();
                bitmapImage.StreamSource = memory;
                bitmapImage.CacheOption = BitmapCacheOption.OnLoad;
                bitmapImage.EndInit();
                bitmapImage.Freeze();

                return bitmapImage;
            }
        }
    }
}

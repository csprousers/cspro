using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.IO;

namespace ToolbarCreator
{
    public class Creator
    {
        private string _inputFilename;
        private string _outputFilename;
        private bool _pngMode;

        public Creator(string inputFilename, string outputFilename = null)
        {
            _inputFilename = inputFilename;
            _outputFilename = ( outputFilename != null) ? outputFilename : GetOutputFilename(_inputFilename);
            _pngMode = ( Path.GetExtension(_outputFilename).ToLower() == ".png" );
        }


        public static string GetOutputFilename(string inputFilename)
        {
            return Path.GetFullPath(Path.Combine(Path.GetDirectoryName(inputFilename), Path.GetFileNameWithoutExtension(inputFilename) + ".bmp"));
        }


        public void Create(bool open_in_paint = true)
        {
            List<Image> images = LoadInputImages();

            if( _pngMode )
            {
                Bitmap toolbar_bitmap = CreateToolbarPng(images);
                toolbar_bitmap.Save(_outputFilename, ImageFormat.Png);
            }

            else
            {
                Bitmap toolbar_bitmap = CreateToolbarBmp(images);
                toolbar_bitmap.Save(_outputFilename, ImageFormat.Bmp);
            }

            // open the resultant bitmap in Paint
            if( open_in_paint )
                Process.Start("mspaint.exe", $"\"{_outputFilename}\"");
        }


        private List<Image> LoadInputImages()
        {
            List<Image> images = new List<Image>();

            var source_directory = new DirectoryInfo(Path.GetDirectoryName(_inputFilename));
            
            foreach( string input_filename in File.ReadAllLines(_inputFilename) )
            {
                var files = source_directory.GetFiles(input_filename, SearchOption.AllDirectories);

                if( files.Length == 0 )
                {
                    throw new Exception($"Could not find {input_filename}");
                }

                else if( files.Length > 1 )
                {
                    throw new Exception($"Too many {input_filename}");
                }

                images.Add(Image.FromFile(files[0].FullName));
            }

            return images;
        }


        private static Bitmap CreateToolbarPng(List<Image> images)
        {
            // each toolbar image will end up as 15x15, with one pixel separating each image,
            // but the source will be 225x225 images
            const int InputSize = 15 * 15;
            const int InputWidthWithMargin = 16 * 16;
            const int OutputSize = 15;
            const int OutputWidthWithMargin = 16;

            var toolbar_bitmap = new Bitmap(InputWidthWithMargin * images.Count, InputSize);

            using( Graphics graphics = Graphics.FromImage(toolbar_bitmap) )
            {
                // draw each icon
                for( int i = 0; i < images.Count; ++i )
                {
                    var image = images[i];

                    if( image.Width != InputSize || image.Height != InputSize )
                        throw new Exception($"Can't work with an image {image.Width}x{image.Height}");

                    graphics.DrawImage(image, i * InputWidthWithMargin, 0, InputSize, InputSize);
                }
            }

            // resize to a ...x15 image
            return ResizeImage(toolbar_bitmap, OutputWidthWithMargin * images.Count, OutputSize, ResizeMode.Bilinear);
        }
        

        private static Bitmap CreateToolbarBmp(List<Image> images)
        {
            // each toolbar image is 16x16, with an additional blank icon at the end;
            // but for now work with the 256x256 bitmaps
            var toolbar_bitmap = new Bitmap(256 * ( images.Count + 1 ), 256);

            using( Graphics graphics = Graphics.FromImage(toolbar_bitmap) )
            {
                // draw the background (using the transparent color)
                graphics.Clear(Color.FromArgb(192, 192, 192));

                // draw each icon
                for( int i = 0; i < images.Count; ++i )
                {
                    var image = images[i];

                    if( image.Width != image.Height )
                        throw new Exception($"Can't work with an image {image.Width}x{image.Height}");

                    if( image.Width != 256 )
                        image = ResizeImage(image, 256, 256, ResizeMode.Icon);
                    
                    graphics.DrawImage(image, i * 256, 0, 256, 256);
                }
            }

            // resize to a ...x16 image
            return ResizeImage(toolbar_bitmap, toolbar_bitmap.Width / 16, 16, ResizeMode.Bilinear);
        }


        enum ResizeMode { Icon, Bilinear }

        private static Bitmap ResizeImage(Image image, int width, int height, ResizeMode? resize_mode)
        {
            // from https://stackoverflow.com/questions/1922040/how-to-resize-an-image-c-sharp
            var destRect = new Rectangle(0, 0, width, height);
            var destImage = new Bitmap(width, height);

            destImage.SetResolution(image.HorizontalResolution, image.VerticalResolution);

            using( var graphics = Graphics.FromImage(destImage) )
            {
                graphics.CompositingMode = CompositingMode.SourceCopy;
                graphics.CompositingQuality = CompositingQuality.HighQuality;
                graphics.InterpolationMode = InterpolationMode.HighQualityBicubic;
                graphics.SmoothingMode = SmoothingMode.HighQuality;
                graphics.PixelOffsetMode = PixelOffsetMode.HighQuality;

                if( resize_mode == ResizeMode.Icon )
                {
                    // settings that seem to work nicely for resampling icons (and keeping the icon pixely)
                    graphics.InterpolationMode = InterpolationMode.NearestNeighbor;
                    graphics.SmoothingMode = SmoothingMode.None;
                    graphics.PixelOffsetMode = PixelOffsetMode.HighSpeed;
                }

                else if( resize_mode == ResizeMode.Bilinear )
                {
                    // trying to match the settings used when this was done with Photoshop
                    graphics.InterpolationMode = InterpolationMode.Bilinear;
                }

                using( var wrapMode = new ImageAttributes() )
                {
                    wrapMode.SetWrapMode(WrapMode.TileFlipXY);
                    graphics.DrawImage(image, destRect, 0, 0, image.Width,image.Height, GraphicsUnit.Pixel, wrapMode);
                }
            }

            return destImage;
        }
    }
}

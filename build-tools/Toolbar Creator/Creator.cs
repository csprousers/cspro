using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.IO;
using Svg;

namespace ToolbarCreator
{
    public class Creator
    {
        private string _inputFilePath;
        private string _outputFilePath;
        private int _buttonWidth;
        private int _buttonHeight;
        private bool _pngMode;

        public Creator(string inputFilePath, string outputFilePath = null, int? buttonWidth = null)
        {
            _inputFilePath = inputFilePath;
            _outputFilePath = ( outputFilePath != null) ? outputFilePath : GetOutputFilePath(_inputFilePath);
            _buttonWidth = buttonWidth.GetValueOrDefault(16);
            _buttonHeight = _buttonWidth - 1;
            _pngMode = ( Path.GetExtension(_outputFilePath).ToLower() == ".png" );
        }


        public static string GetOutputFilePath(string inputFilePath)
        {
            return Path.GetFullPath(Path.Combine(Path.GetDirectoryName(inputFilePath), Path.GetFileNameWithoutExtension(inputFilePath) + ".bmp"));
        }


        public void Create(bool open_in_paint = true)
        {
            List<Image> images = LoadInputImages();

            if( _pngMode )
            {
                Bitmap toolbar_bitmap = CreateToolbarPng(images);
                toolbar_bitmap.Save(_outputFilePath, ImageFormat.Png);
            }

            else
            {
                Bitmap toolbar_bitmap = CreateToolbarBmp(images);
                toolbar_bitmap.Save(_outputFilePath, ImageFormat.Bmp);
            }

            // open the resultant bitmap in Paint
            if( open_in_paint )
                Process.Start("mspaint.exe", $"\"{_outputFilePath}\"");
        }


        private List<Image> LoadInputImages()
        {
            List<Image> images = new List<Image>();

            var source_directory = new DirectoryInfo(Path.GetDirectoryName(_inputFilePath));

            foreach( string line in File.ReadAllLines(_inputFilePath) )
            {
                // strip any comments
                int comment_pos = line.IndexOf("//");
                string input_filename = ( comment_pos >= 0 ) ? line.Substring(0, comment_pos).Trim() :
                                                               line.Trim();

                var files = source_directory.GetFiles(input_filename, SearchOption.AllDirectories);

                if( files.Length == 0 )
                {
                    throw new Exception($"Could not find {input_filename}");
                }

                else if( files.Length > 1 )
                {
                    throw new Exception($"Too many {input_filename}");
                }

                if( files[0].Extension.ToLower().Equals(".svg") )
                {
                    images.Add(ConvertSvgToBitmap(files[0].FullName));
                }

                else
                {
                    images.Add(Image.FromFile(files[0].FullName));
                }
            }

            return images;
        }


        enum CropType { TopRow, BottomRow, Resize };

        private Bitmap ConvertSvgToBitmap(string file_path)
        {
            SvgDocument svg_doc = SvgDocument.Open(file_path);

            if( svg_doc.Width != svg_doc.Height )
                throw new Exception($"Can't work with a SVG with a width ({svg_doc.Width}) different from its height ({svg_doc.Height}).");

            // draw the SVG in the width of the button
            Bitmap button_width_image = svg_doc.Draw(_buttonWidth, _buttonWidth);

            // because the height of the button is one less than the width, crop the image as necessary
            Func<Bitmap, int, bool> IsRowTransparent = (bitmap, row) =>
            {
                for( int x = 0; x < bitmap.Width; ++x )
                {
                    if( bitmap.GetPixel(x, row).A != 0 )
                        return false;
                }

                return true;
            };

            CropType crop_type = IsRowTransparent(button_width_image, _buttonHeight) ? CropType.BottomRow :
                                 IsRowTransparent(button_width_image, 0)             ? CropType.TopRow :
                                                                                       CropType.Resize;

            return _pngMode ? ConvertSvgToBitmapForPng(svg_doc, button_width_image, crop_type) :
                              ConvertSvgToBitmapForBmp(svg_doc, crop_type);
        }


        private Bitmap ConvertSvgToBitmapForPng(SvgDocument svg_doc, Bitmap button_width_image, CropType crop_type)
        {
            Bitmap cropped_image = new Bitmap(_buttonWidth, _buttonHeight);

            using( Graphics graphics = Graphics.FromImage(cropped_image) )
            {
                Rectangle source_rect;

                // crop the top row
                if( crop_type == CropType.TopRow )
                {
                    source_rect = new Rectangle(0, 1, _buttonWidth, _buttonHeight);
                }

                // crop the bottom row
                else if( crop_type == CropType.BottomRow )
                {
                    source_rect = new Rectangle(0, 0, _buttonWidth, _buttonHeight);
                }

                // or redraw to the destination height
                else
                {
                    button_width_image = svg_doc.Draw(_buttonHeight, _buttonHeight);
                    source_rect = new Rectangle(0, 0, _buttonHeight, _buttonHeight);
                }

                graphics.DrawImage(button_width_image, 0, 0, source_rect, GraphicsUnit.Pixel);
            }

            return cropped_image;
        }


        private static Bitmap ConvertSvgToBitmapForBmp(SvgDocument svg_doc, CropType crop_type)
        {
            int eventual_button_dimension = ( crop_type == CropType.Resize ) ? 15 : 16;
            int dimension_to_draw = eventual_button_dimension * eventual_button_dimension;

            Bitmap source_image = svg_doc.Draw(dimension_to_draw, dimension_to_draw);
            Rectangle source_rect = new Rectangle(0, 0, dimension_to_draw, dimension_to_draw);

            Bitmap cropped_image = new Bitmap(256, 256);

            using( Graphics graphics = Graphics.FromImage(cropped_image) )
            {
                // crop the top row
                if( crop_type == CropType.TopRow )
                {
                    source_rect.Y = eventual_button_dimension;
                    source_rect.Height -= eventual_button_dimension;
                }

                // crop the bottom row
                else if( crop_type == CropType.BottomRow )
                {
                    source_rect.Height -= eventual_button_dimension;
                }

                graphics.DrawImage(source_image, 0, 0, source_rect, GraphicsUnit.Pixel);
            }

            return cropped_image;
        }


        private Bitmap CreateToolbarPng(List<Image> images)
        {
            // this routine has been tested creating 16x16 and 20x20 buttons
            var toolbar_bitmap = new Bitmap(_buttonWidth * images.Count, _buttonHeight);

            using( Graphics graphics = Graphics.FromImage(toolbar_bitmap) )
            {
                // resize and draw each icon
                for( int i = 0; i < images.Count; ++i )
                {
                    Image image = images[i];

                    if( image.Width != _buttonWidth || image.Height != _buttonHeight )
                    {
                        if( image.Width != image.Height )
                            throw new Exception($"Can't work with an image with a width ({image.Width}) different from its height ({image.Height}).");

                        image = ResizeImage(image, _buttonHeight, _buttonHeight, ResizeMode.Bilinear);
                    }

                    Debug.Assert(image.Width == _buttonWidth || image.Width == _buttonHeight);
                    Debug.Assert(image.Height == _buttonHeight);

                    graphics.DrawImage(image, i * _buttonWidth, 0, image.Width, image.Height);
                }
            }

            return toolbar_bitmap;
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
                    Image image = images[i];

                    if( image.Width != image.Height )
                        throw new Exception($"Can't work with an image {image.Width}x{image.Height}.");

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

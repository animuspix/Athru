using System;
using System.Collections.Generic;
using System.Drawing;

namespace TectoBuilder
{
    class Program
    {
        class PlateBorders
        {
            public PlateBorders(Bitmap plateBorderImage, List<Pixel> plateBorderpoints)
            {
                borderImage = plateBorderImage;
                borderPoints = plateBorderpoints;
            }

            public Bitmap borderImage;
            public List<Pixel> borderPoints;
        }

        class ColorVec
        {
            public ColorVec(Color buildFrom)
            {
                rgbaVals = new float[4];
                rgbaVals[0] = 0;
                rgbaVals[0] = 0;
                rgbaVals[0] = 0;
                rgbaVals[0] = 0;

                if (buildFrom.R != 0)
                {
                    rgbaVals[0] = (float)buildFrom.R / 255;
                }

                if (buildFrom.G != 0)
                {
                    rgbaVals[1] = (float)buildFrom.G / 255;
                }

                if (buildFrom.B != 0)
                {
                    rgbaVals[2] = (float)buildFrom.B / 255;
                }

                if (buildFrom.A != 0)
                {
                    rgbaVals[3] = (float)buildFrom.A / 255;
                }
            }

            public void SumWith(ColorVec x)
            {
                rgbaVals[0] += x.rgbaVals[0];
                rgbaVals[1] += x.rgbaVals[1];
                rgbaVals[2] += x.rgbaVals[2];
                rgbaVals[3] += x.rgbaVals[3];
            }

            public void ScaleBy(float x)
            {
                rgbaVals[0] *= x;
                rgbaVals[1] *= x;
                rgbaVals[2] *= x;
                rgbaVals[3] *= x;
            }

            public void ScaleBy(ColorVec x)
            {
                rgbaVals[0] *= x.rgbaVals[0];
                rgbaVals[1] *= x.rgbaVals[1];
                rgbaVals[2] *= x.rgbaVals[2];
                rgbaVals[3] *= x.rgbaVals[3];
            }

            public void SqrRoot()
            {
                rgbaVals[0] = (float)Math.Sqrt(rgbaVals[0]);
                rgbaVals[1] = (float)Math.Sqrt(rgbaVals[1]);
                rgbaVals[2] = (float)Math.Sqrt(rgbaVals[2]);
                rgbaVals[3] = (float)Math.Sqrt(rgbaVals[3]);
            }

            public Color GetColor()
            {
                int r = Math.Max(((int)(255 * rgbaVals[0]) % 255), 0);
                int g = Math.Max(((int)(255 * rgbaVals[1]) % 255), 0);
                int b = Math.Max(((int)(255 * rgbaVals[2]) % 255), 0);
                int a = Math.Max(((int)(255 * rgbaVals[3]) % 255), 0);
                return Color.FromArgb(a, r, g, b);
            }

            public float[] rgbaVals;
        }

        class Pixel
        {
            public Pixel()
            {
                pos = new Point(0, 0);
                color = Color.Bisque;
            }

            public Point pos;
            public Color color;
        };

        static int Sqr(int x)
        {
            return x * x;
        }

        static float DistToPoint(Point from, Point to)
        {
            Point diffVec = new Point(to.X - from.X, to.Y - from.Y);
            return (float)Math.Sqrt(Sqr(diffVec.X) + Sqr(diffVec.Y));
        }

        static Pixel GetClosest(Point pos, Pixel[] searchSet)
        {
            Pixel closestPix = searchSet[0];
            float minDist = (float)Math.Pow(2, 10);
            foreach (Pixel currPix in searchSet)
            {
                float distToCurrPix = DistToPoint(pos, currPix.pos);
                if (distToCurrPix < minDist)
                {
                    minDist = distToCurrPix;
                    closestPix = currPix;
                }
            }

            return closestPix;
        }

        static PlateBorders MarkBorders(Bitmap tectoPlates, int imageWidth)
        {
            PlateBorders tectoPlateBorders = new PlateBorders(new Bitmap(imageWidth, imageWidth), new List<Pixel>());
            for (int i = 0; i < imageWidth; i += 1)
            {
                for (int j = 0; j < imageWidth; j += 1)
                {
                    if (i > 0 && j > 0 &&
                        i < 511 && j < 511)
                    {
                        if (tectoPlates.GetPixel(i, j).ToArgb() != 0)
                        {
                            // Initialise sobel X matrix

                            // Row 1
                            ColorVec sobelXA = new ColorVec(tectoPlates.GetPixel(i - 1, j - 1));
                            ColorVec sobelXB = new ColorVec(tectoPlates.GetPixel(i, j - 1));
                            ColorVec sobelXC = new ColorVec(tectoPlates.GetPixel(i + 1, j - 1));

                            sobelXA.ScaleBy(-1);
                            sobelXB.ScaleBy(0);
                            sobelXC.ScaleBy(1);

                            // Row 2
                            ColorVec sobelXD = new ColorVec(tectoPlates.GetPixel(i - 1, j));
                            ColorVec sobelXE = new ColorVec(tectoPlates.GetPixel(i, j));
                            ColorVec sobelXF = new ColorVec(tectoPlates.GetPixel(i + 1, j));

                            sobelXD.ScaleBy(-2);
                            sobelXE.ScaleBy(0);
                            sobelXF.ScaleBy(2);

                            // Row 3
                            ColorVec sobelXG = new ColorVec(tectoPlates.GetPixel(i - 1, j + 1));
                            ColorVec sobelXH = new ColorVec(tectoPlates.GetPixel(i, j + 1));
                            ColorVec sobelXI = new ColorVec(tectoPlates.GetPixel(i + 1, j + 1));

                            sobelXG.ScaleBy(-1);
                            sobelXH.ScaleBy(0);
                            sobelXI.ScaleBy(1);

                            ColorVec sobelX = sobelXA;
                            sobelX.SumWith(sobelXB);
                            sobelX.SumWith(sobelXC);
                            sobelX.SumWith(sobelXD);
                            sobelX.SumWith(sobelXE);
                            sobelX.SumWith(sobelXF);
                            sobelX.SumWith(sobelXG);
                            sobelX.SumWith(sobelXH);
                            sobelX.SumWith(sobelXI);

                            // Initialise sobel Y matrix

                            // Row 1
                            ColorVec sobelYA = new ColorVec(tectoPlates.GetPixel(i - 1, j - 1));
                            ColorVec sobelYB = new ColorVec(tectoPlates.GetPixel(i, j - 1));
                            ColorVec sobelYC = new ColorVec(tectoPlates.GetPixel(i + 1, j - 1));

                            sobelYA.ScaleBy(-1);
                            sobelYB.ScaleBy(-2);
                            sobelYC.ScaleBy(-1);

                            // Row 2
                            ColorVec sobelYD = new ColorVec(tectoPlates.GetPixel(i - 1, j));
                            ColorVec sobelYE = new ColorVec(tectoPlates.GetPixel(i, j));
                            ColorVec sobelYF = new ColorVec(tectoPlates.GetPixel(i + 1, j));

                            sobelYD.ScaleBy(0);
                            sobelYE.ScaleBy(0);
                            sobelYF.ScaleBy(0);

                            // Row 3
                            ColorVec sobelYI = new ColorVec(tectoPlates.GetPixel(i - 1, j + 1));
                            ColorVec sobelYG = new ColorVec(tectoPlates.GetPixel(i, j + 1));
                            ColorVec sobelYH = new ColorVec(tectoPlates.GetPixel(i + 1, j + 1));

                            sobelYG.ScaleBy(1);
                            sobelYH.ScaleBy(2);
                            sobelYI.ScaleBy(1);

                            ColorVec sobelY = sobelYA;
                            sobelY.SumWith(sobelYB);
                            sobelY.SumWith(sobelYC);
                            sobelY.SumWith(sobelYD);
                            sobelY.SumWith(sobelYE);
                            sobelY.SumWith(sobelYF);
                            sobelY.SumWith(sobelYG);
                            sobelY.SumWith(sobelYH);
                            sobelY.SumWith(sobelYI);

                            // Compute X^2
                            ColorVec sobelSqrXA = sobelX;
                            sobelSqrXA.ScaleBy(sobelX);

                            // Compute Y^2
                            ColorVec sobelSqrYA = sobelY;
                            sobelSqrYA.ScaleBy(sobelY);

                            // Compute X^2 + Y^2
                            ColorVec sobelSumXYA = sobelSqrXA;
                            sobelSumXYA.SumWith(sobelSqrYA);

                            // Compute sqrt(X^2 + Y^2) (i.e. the final Sobel-filtered pixel)
                            ColorVec sobel = sobelSumXYA;
                            sobel.SqrRoot();

                            // If the sobel operation produced a non-zero pixel (i.e. the current pixel sits on an edge),
                            // apply a grayscale filter by distributing the red channel throughout the output pixel
                            // Also register the generated point so we can do altitude interpolation later on :)
                            if (sobel.rgbaVals[0] > 0.01f ||
                                sobel.rgbaVals[1] > 0.01f ||
                                sobel.rgbaVals[2] > 0.01f ||
                                sobel.rgbaVals[3] > 0.01f)
                            {
                                sobel.rgbaVals[1] = sobel.rgbaVals[0];
                                sobel.rgbaVals[2] = sobel.rgbaVals[0];
                                sobel.rgbaVals[3] = sobel.rgbaVals[0];

                                Pixel sobelPix = new Pixel
                                {
                                    color = sobel.GetColor(),
                                    pos = new Point(i, j)
                                };

                                tectoPlateBorders.borderPoints.Add(sobelPix);
                            }

                            // Write the filtered pixel into the relevant image
                            tectoPlateBorders.borderImage.SetPixel(i, j, sobel.GetColor());
                        }
                    }
                }
            }

            return tectoPlateBorders;
        }

        static Bitmap RefillPlates(Pixel[] borderPoints, int numBorderPoints,
                                   int imageWidth, Point centrePoint)
        {
            Bitmap lerpedTectoPlates = new Bitmap(imageWidth, imageWidth);

            // More voronoi! Generate height data by creating a voronoi diagram
            // from points along the edges of the cells created during initial
            // plate generation

            // Generate voronoi diagram
            for (int i = 0; i < imageWidth; i += 1)
            {
                for (int j = 0; j < imageWidth; j += 1)
                {
                    // Avoid shading areas outside the projective circle
                    Point currPoint = new Point(i, j);
                    if (DistToPoint(currPoint, centrePoint) < ((imageWidth / 2) - 2))
                    {
                        Pixel closestEdge = GetClosest(currPoint, borderPoints);
                        //ColorVec currColor = new ColorVec(closestEdge.color);

                        // Interpolate between the displacement of the nearest edge
                        // and sea level
                        float seaLevel = 128;
                        float closestColorMag = closestEdge.color.R;
                        float distFract = 1 / (DistToPoint(currPoint, closestEdge.pos) + 1);
                        int freshColorMag = (int)((seaLevel * (1.0f - distFract)) + (closestColorMag * distFract));

                        // Write the interpolated pixel into the output [Bitmap]
                        lerpedTectoPlates.SetPixel(i, j, Color.FromArgb(freshColorMag, freshColorMag, freshColorMag, freshColorMag));
                    }
                }
            }

            return lerpedTectoPlates;
        }

        static void Main(string[] args)
        {
            // Algorithm to generate crude geological plates from a
            // limited-color Voronoi Diagram (https://en.wikipedia.org/wiki/Voronoi_diagram)
            int width = 512;
            Bitmap tectoPlatesLower = new Bitmap(width, width);
            Bitmap tectoPlatesUpper = new Bitmap(width, width);

            int numTectoPoints = 16;
            Pixel[] tectoPoints = new Pixel[numTectoPoints];

            // Array of valid cell colors
            Color[] cellColors = new Color[numTectoPoints];
            cellColors[0] = Color.Aquamarine;
            cellColors[1] = Color.Coral;
            cellColors[2] = Color.Chartreuse;
            cellColors[3] = Color.Khaki;

            // Generate points
            // Randomly color initial points for easy identification
            Random rand = new Random();
            for (int i = 0; i < numTectoPoints; i += 1)
            {
                tectoPoints[i] = new Pixel();
                tectoPoints[i].pos.X = rand.Next(0, width);
                tectoPoints[i].pos.Y = rand.Next(0, width);
                tectoPoints[i].color = Color.FromArgb(1, rand.Next(0, 255), rand.Next(0, 255), rand.Next(0, 255));//cellColors[rand.Next(0, 4)];
            }

            // Generate upper projective circle (corresponds to the northern hemisphere)
            for (int i = 0; i < width; i += 1)
            {
                for (int j = 0; j < width; j += 1)
                {
                    Point currPoint = new Point(i, j);
                    tectoPlatesUpper.SetPixel(i, j, GetClosest(currPoint, tectoPoints).color);
                }
            }

            // Strip colors lying outside the projective circle itself
            // Make sure to add a few pixels worth of padding so the circle
            // isn't clipped by the edge of the image
            Point centrePoint = new Point(width / 2, width / 2);
            for (int i = 0; i < width; i += 1)
            {
                for (int j = 0; j < width; j += 1)
                {
                    Point currPoint = new Point(i, j);
                    if (DistToPoint(currPoint, centrePoint) > ((width / 2) - 2))
                    {
                        tectoPlatesUpper.SetPixel(i, j, Color.FromArgb(0, 0, 0, 0));
                    }
                }
            }

            // Generate lower projective circle (corresponds to the southern hemisphere)
            for (int i = 0; i < width; i += 1)
            {
                for (int j = 0; j < width; j += 1)
                {
                    Point currPoint = new Point(i, j);
                    tectoPlatesLower.SetPixel(i, j, tectoPlatesUpper.GetPixel(width - i - (1 * Convert.ToByte(i == 0)), j));
                }
            }

            // Define borders with (modified) Sobel Edge Detection (https://en.wikipedia.org/wiki/Sobel_operator)
            // Differing border brightnesses are interpreted as differing tectonic displacements
            PlateBorders borderImageUpper = MarkBorders(tectoPlatesUpper, width);
            PlateBorders borderImageLower = MarkBorders(tectoPlatesLower, width);

            // Fill in cell altitudes by interpolating between border displacements
            Bitmap tectoImageUpper = RefillPlates(borderImageUpper.borderPoints.ToArray(), borderImageUpper.borderPoints.Count,
                                                  width, centrePoint);

            Bitmap tectoImageLower = RefillPlates(borderImageLower.borderPoints.ToArray(), borderImageLower.borderPoints.Count,
                                                  width, centrePoint);

            // Copy tectonic data back into the projective circles
            for (int i = 0; i < width; i += 1)
            {
                for (int j = 0; j < width; j += 1)
                {
                    tectoPlatesUpper.SetPixel(i, j, tectoImageUpper.GetPixel(i, j));
                    tectoPlatesLower.SetPixel(i, j, tectoImageLower.GetPixel(i, j));
                }
            }

            tectoPlatesLower.Save("tectoLower.bmp", System.Drawing.Imaging.ImageFormat.Bmp);
            tectoPlatesUpper.Save("tectoUpper.bmp", System.Drawing.Imaging.ImageFormat.Bmp);
        }
    }
}

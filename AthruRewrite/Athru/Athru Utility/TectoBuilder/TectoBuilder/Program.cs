using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Drawing;

namespace TectoBuilder
{
    class Program
    {
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
            Point centrePoint = new Point(width / 2, width / 2);
            for (int i = 0; i < width; i += 1)
            {
                for (int j = 0; j < width; j += 1)
                {
                    Point currPoint = new Point(i, j);
                    if (DistToPoint(currPoint, centrePoint) > (width / 2))
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

            // Color borders with proportional displacement values balanced about 128 (representing cumulative tectonic drift)
            // (0 = -100%, 255 = +100%)
            for (int i = 0; i < width; i += 1)
            {
                for (int j = 0; j < width; j += 1)
                {
                    if (i > 0 && j > 0 &&
                        j < 511 && j < 511)
                    {
                        // Color upper projective circle
                        Color upperTectoLastPixY = tectoPlatesUpper.GetPixel(i, j - 1);
                        Color upperTectoCurrPixY = tectoPlatesUpper.GetPixel(i, j);
                        Color upperTectoNextPixY = tectoPlatesUpper.GetPixel(i, j + 1);
            
                        if (upperTectoLastPixY != Color.FromArgb(0, 0, 0, 0) &&
                            upperTectoCurrPixY != Color.FromArgb(0, 0, 0, 0) &&
                            upperTectoNextPixY != Color.FromArgb(0, 0, 0, 0))
                        {
                            if (upperTectoLastPixY != upperTectoNextPixY)
                            {
                                tectoPlatesUpper.SetPixel(i, j, Color.FromArgb(rand.Next(0, 255), rand.Next(0, 255), rand.Next(0, 255), rand.Next(0, 255)));
                            }
                        }
            
                        // Color lower projective circle
                        Color lowerTectoLastPixY = tectoPlatesLower.GetPixel(i, j - 1);
                        Color lowerTectoCurrPixY = tectoPlatesLower.GetPixel(i, j);
                        Color lowerTectoNextPixY = tectoPlatesLower.GetPixel(i, j + 1);
            
                        if (lowerTectoLastPixY != Color.FromArgb(0, 0, 0, 0) &&
                            lowerTectoCurrPixY != Color.FromArgb(0, 0, 0, 0) &&
                            lowerTectoNextPixY != Color.FromArgb(0, 0, 0, 0))
                        {
                            if (lowerTectoLastPixY != lowerTectoNextPixY)
                            {
                                tectoPlatesLower.SetPixel(i, j, Color.FromArgb(rand.Next(0, 255), rand.Next(0, 255), rand.Next(0, 255), rand.Next(0, 255)));
                            }
                        }
                    }
                }
            }

            // Fill in cell altitudes by interpolating between border displacements

            tectoPlatesLower.Save("tectoLower.bmp", System.Drawing.Imaging.ImageFormat.Bmp);
            tectoPlatesUpper.Save("tectoUpper.bmp", System.Drawing.Imaging.ImageFormat.Bmp);
        }
    }
}

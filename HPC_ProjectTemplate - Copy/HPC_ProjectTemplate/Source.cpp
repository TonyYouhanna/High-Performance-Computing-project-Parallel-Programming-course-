#include <mpi.h>
#include <stdio.h>
#include <iostream>
#include <math.h>
#include <stdlib.h>
#include<string.h>
#include<msclr\marshal_cppstd.h>
#include <ctime>// include this header 
#pragma once

#using <mscorlib.dll>
#using <System.dll>
#using <System.Drawing.dll>
#using <System.Windows.Forms.dll>
using namespace std;
using namespace msclr::interop;

int* inputImage(int* w, int* h, System::String^ imagePath) //put the size of image in w & h
{
	int* input;


	int OriginalImageWidth, OriginalImageHeight;

	//*********************************************************Read Image and save it to local arrayss*************************	
	//Read Image and save it to local arrayss

	System::Drawing::Bitmap BM(imagePath);

	OriginalImageWidth = BM.Width;
	OriginalImageHeight = BM.Height;
	*w = BM.Width;
	*h = BM.Height;
	int *Red = new int[BM.Height * BM.Width];
	int *Green = new int[BM.Height * BM.Width];
	int *Blue = new int[BM.Height * BM.Width];
	input = new int[BM.Height*BM.Width];
	for (int i = 0; i < BM.Height; i++)
	{
		for (int j = 0; j < BM.Width; j++)
		{
			System::Drawing::Color c = BM.GetPixel(j, i);

			Red[i * BM.Width + j] = c.R;
			Blue[i * BM.Width + j] = c.B;
			Green[i * BM.Width + j] = c.G;

			input[i*BM.Width + j] = ((c.R + c.B + c.G) / 3); //gray scale value equals the average of RGB values

		}

	}
	return input;
}


void createImage(int* image, int width, int height, int index)
{
	System::Drawing::Bitmap MyNewImage(width, height);


	for (int i = 0; i < MyNewImage.Height; i++)
	{
		for (int j = 0; j < MyNewImage.Width; j++)
		{
			//i * OriginalImageWidth + j
			if (image[i*width + j] < 0)
			{
				image[i*width + j] = 0;
			}
			if (image[i*width + j] > 255)
			{
				image[i*width + j] = 255;
			}
			System::Drawing::Color c = System::Drawing::Color::FromArgb(image[i*MyNewImage.Width + j], image[i*MyNewImage.Width + j], image[i*MyNewImage.Width + j]);
			MyNewImage.SetPixel(j, i, c);
		}
	}
	MyNewImage.Save("..//Data//Output//outputRes" + index + ".jpg");
	cout << "result Image Saved " << index << endl;
}
static string ext = ".jpg";
int** create_array_of_images(int n, int* w, int* h)
{
	int** array_of_images = new int* [n];
	System::String^ imagePath;
	string img, image_num, image;
	for (int i = 1; i <= n; i++)
	{
		image_num = to_string(i);
		if (i <= 9)
		{
			image = "in00000" + image_num + ext;
		}
		else if (i > 9 && i <= 99)
		{
			image = "in0000" + image_num + ext;
		}
		else if (i > 99)
		{
			image = "in000" + image_num + ext;
		}
		img = "..//Data//Input//" + image;

		imagePath = marshal_as<System::String^>(img);
		int* imageData = inputImage(&*w, &*h, imagePath);

		array_of_images[i - 1] = new int[*w * *h];
		array_of_images[i - 1] = imageData;


	}

	return array_of_images;
}
int main()
{
	

	int start_s, stop_s, TotalTime = 0;

	
	
	start_s = clock();

	MPI_Init(NULL, NULL);
	
    int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	
	int w = 0, h = 0;
	int full_size_of_img = 0;
	int** array_of_images = 0;
	int* random_image = 0;
	int number_of_images;
	int threshold;

	if (world_rank == 0)
	{
		
		cout << "Enter number of images: ";
		cin >> number_of_images;
		cout << "Enter threshold value: ";
		cin >> threshold;
		array_of_images = create_array_of_images(number_of_images, &w, &h);

		
		string img = "..//Data//Input//in000154" + ext;
		System::String^ imagePath = marshal_as<System::String^>(img);
		random_image = inputImage(&w, &h, imagePath);

		full_size_of_img = w * h;

	}
	MPI_Bcast(&number_of_images, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&full_size_of_img, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&threshold, 1, MPI_INT, 0, MPI_COMM_WORLD);

	int** local_array_of_images = new int* [number_of_images];
	int* Resultbackgroundimage = new int[full_size_of_img];
	int* object_filtering = new int[full_size_of_img];
	int local_arr_size = (full_size_of_img) / world_size;

	for (int i = 0; i < number_of_images; i++)
	{
		if (world_rank == 0)
		{
			for (int j = 1; j < world_size; j++)
			{
				MPI_Send(&array_of_images[i][j * local_arr_size], local_arr_size, MPI_INT, j, 0, MPI_COMM_WORLD);
			}

			
			int sum = 0;
			for (int x = 0; x < local_arr_size; x++)
			{
				for (int k = 0; k < number_of_images; k++)
				{
					sum += array_of_images[k][x];
				}
				sum /= number_of_images;
				Resultbackgroundimage[x] = sum;
				int diff = abs(Resultbackgroundimage[x] - random_image[x]);
				if (diff > threshold)
				{
					object_filtering[x] = diff;
				}

				else
				{
					object_filtering[x] = 0;
				}
			}
			
			if (i == number_of_images - 1)
			{
				for (int k = 1; k < world_size; k++)
				{
					MPI_Status status;
					MPI_Send(&random_image[k * local_arr_size], local_arr_size, MPI_INT, k, 0, MPI_COMM_WORLD);
					MPI_Recv(&Resultbackgroundimage[k * local_arr_size], local_arr_size, MPI_INT, k, 0, MPI_COMM_WORLD, &status);
					MPI_Recv(&object_filtering[k * local_arr_size], local_arr_size, MPI_INT, k, 0, MPI_COMM_WORLD, &status);
				}
				createImage(Resultbackgroundimage, w, h, 0);
				createImage(object_filtering, w, h, 1);
			}
		}
		else if (world_rank <= world_size)
		{
			
			local_array_of_images[i] = new int[local_arr_size];

			MPI_Status status;
			MPI_Recv(local_array_of_images[i], local_arr_size, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);


			
			if (i == number_of_images - 1)
			{
				int* part_of_random_image = new int[local_arr_size];
				MPI_Recv(part_of_random_image, local_arr_size, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

				int sum = 0;
				int* localbackgroundimage = new int[local_arr_size];
				int* local_object_filtering = new int[local_arr_size];

				for (int x = 0; x < local_arr_size; x++)
				{
					for (int k = 0; k < number_of_images; k++)
					{
						sum += local_array_of_images[k][x];
					}
					sum /= number_of_images;
					localbackgroundimage[x] = sum;
					int diff = abs(localbackgroundimage[x] - part_of_random_image[x]);
					if (diff > threshold)
					{
						local_object_filtering[x] = diff;

					}
					else
					{
						local_object_filtering[x] = 0;
					}
				}
				MPI_Send(localbackgroundimage, local_arr_size, MPI_INT, 0, 0, MPI_COMM_WORLD);
				MPI_Send(local_object_filtering, local_arr_size, MPI_INT, 0, 0, MPI_COMM_WORLD);
			}
		}
	}
	MPI_Finalize();
	system("pause");
	stop_s = clock();
	TotalTime += (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000;
	cout << "time: " << TotalTime << endl;
	return 0;
}




#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <algorithm>

typedef unsigned char uint8_t;
typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int uint32_t;

class BMP_FILE;
class FILTER;

float median(float* array, int size);

typedef enum{
    RGB_FIELD = 1,
    CIE_FIELD = 0
} FIELD;

typedef struct{
	uint8_t a;
	uint8_t r;
	uint8_t g;
	uint8_t b;
} RGB_PIXEL;

typedef struct{
    uint8_t a;
    float   y;
    float   b;
    float   r;
} YCbCr_PIXEL;

typedef struct BMP_HEADER{
    char name[2];
    unsigned int size;
    unsigned int reserved;
    unsigned int image_offset; //offset from where image starts in file
} BMP_HEADER;

typedef struct DIB_HEADER{
	unsigned int size;
	unsigned int width;
	unsigned int height;
	unsigned short int number_color_plane;
	unsigned short int number_bit_per_pixel;
	unsigned int compression_method;
	unsigned int image_size;
	unsigned int horizontal_resolution;
	unsigned int vertical_resolution;
	unsigned int number_color;
	unsigned int ignored;
} DIB_HEADER;

class FILTER
{
private:
    friend BMP_FILE;
    int size;
    int seperable;
    int symmetric;
public:
    float* filter;
    FILTER(int in_size, int in_seperable, int symmetric);
    ~FILTER();
};

class BMP_FILE
{
private:
//	friend 		BMP_FILE;
    int         builded;
    FIELD       field;
    BMP_HEADER  file_header;
    DIB_HEADER  dib_header;
    RGB_PIXEL*  rgb;
    YCbCr_PIXEL*  xyz;
public:
    BMP_FILE();
    ~BMP_FILE();
    int readfile(char* file_name);
    int createfile(char* file_name);
    int rgb2xyz();
    int xyz2rgb();
    int histogramming(float weight, BMP_FILE& output);
    int convolution(FILTER& filter, BMP_FILE& output);
    int medianfilter(int size, BMP_FILE& output);
    int medianfilter_convolution(int size, FILTER& filter, BMP_FILE& output);
};

int main()
{	
	BMP_FILE input_bmp, output_bmp;
	
	char input_file_name[] = "input1.bmp";
	char output_file_name_1[] = "output1_1.bmp";
	char output_file_name_2[] = "output1_2.bmp";
	
	input_bmp.readfile(input_file_name);
	input_bmp.rgb2xyz();
	
	input_bmp.histogramming(0.5, output_bmp);
	output_bmp.xyz2rgb();
	output_bmp.createfile(output_file_name_1); 
	output_bmp.~BMP_FILE();
	
	input_bmp.histogramming(1.0, output_bmp);
	output_bmp.xyz2rgb();
	output_bmp.createfile(output_file_name_2);
	output_bmp.~BMP_FILE();
	
	input_bmp.~BMP_FILE();
	
	return 0;
}

BMP_FILE::BMP_FILE()
{
    builded  = 0;
}

BMP_FILE::~BMP_FILE()
{
    if(builded)
    {
        free(rgb);
        free(xyz);
        builded = 0;
    }
}

int BMP_FILE::readfile(char* file_name)
/*
*   return value
*       1: The BMP_FILE hase been builded
*       2: Cannot open file
*       3: The file doesn't bmp file
*/
{
    if(builded)
    {
        return 1;
    }
    else
    {
        FILE* bmp_file;
        bmp_file = fopen(file_name, "rb");
        if(bmp_file == NULL) return 2;

        // Read file header
        fread(file_header.name, 2, 1, bmp_file);
        fread(&file_header.size, 3*sizeof(int), 1, bmp_file);
        if((file_header.name[0] != 'B')||(file_header.name[1] != 'M'))
        {
            fclose(bmp_file);
            return 3;
        }

        // Read dib header
        fread(&dib_header, 40, 1, bmp_file);
        if((dib_header.size != 40)||(dib_header.compression_method != 0))
        {
            fclose(bmp_file);
            return 1;
        }

        fseek(bmp_file, file_header.image_offset, SEEK_SET);

        // Calculate pixels of file
        uint32_t size_image_content, i, this_pixel;
        size_image_content = dib_header.width * dib_header.height;
        
        // Build pixels
        rgb = (RGB_PIXEL*) malloc (size_image_content * sizeof(RGB_PIXEL));
        xyz = (YCbCr_PIXEL*) malloc (size_image_content * sizeof(YCbCr_PIXEL));
        builded = 1; 
        field = RGB_FIELD;

        for(i = 0; i < size_image_content; i++)
        {
            // small endian
            if(dib_header.number_bit_per_pixel == 24)
            {
                fread(&this_pixel, 3, 1, bmp_file);
                rgb[i].b = this_pixel & 0xff;
                rgb[i].g = (this_pixel & 0xff00) >> 8;
                rgb[i].r = (this_pixel & 0xff0000) >> 16;
                rgb[i].a = 255;
            }
            else if(dib_header.number_bit_per_pixel == 32)
            {
                fread(&this_pixel, 4, 1, bmp_file);
                rgb[i].b = this_pixel & 0xff;
                rgb[i].g = (this_pixel & 0xff00) >> 8;
                rgb[i].r = (this_pixel & 0xff0000) >> 16;
                rgb[i].a = (this_pixel & 0xff000000) >> 24;
            }
            //printf("a:%03d r:%03d g:%03d b:%03d\n", rgb[i].a, rgb[i].r, rgb[i].g, rgb[i].b);
        }
        fclose(bmp_file);
        return 0;
    }
    return 0;
}

int BMP_FILE::createfile(char* file_name)
{
    if(builded == 1)
    {
        // Check field
        if(field == CIE_FIELD)
        {
            printf("Please turn to RGB field before create file!");
            return 1;
        }
        // Open file
        FILE* file = fopen(file_name, "wb");
        if(file == NULL) return 2;
        
        // Write file header and dib header
        fwrite(file_header.name, 2, 1, file);
        fwrite(&file_header.size, 12, 1, file);
        fwrite(&dib_header, 40, 1, file);
        
        fseek(file, file_header.image_offset, SEEK_SET);
        
        // Calculate pixels of file
        uint32_t size_image_content, i, this_pixel;
        size_image_content = dib_header.width * dib_header.height;
        
        // Write pixels
        for(i = 0; i < size_image_content; i++)
        {
            if(dib_header.number_bit_per_pixel == 24)
            {
                this_pixel = 0;
                this_pixel |= rgb[i].b;
                this_pixel |= rgb[i].g << 8;
                this_pixel |= rgb[i].r << 16;
                fwrite(&(this_pixel), 3, 1, file);
            }
            else if(dib_header.number_bit_per_pixel == 32)
            {
                this_pixel  = rgb[i].b;
                this_pixel |= rgb[i].g << 8;
                this_pixel |= rgb[i].r << 16;
                this_pixel |= rgb[i].a << 24;
                fwrite(&(this_pixel), 4, 1, file);
            }
        }
        fclose(file);
    }
    else
    {
        return 1;
    }
	return 0;
}

int BMP_FILE::rgb2xyz()
{
    if(!builded)
    {
        return 1;
    }
    if(field == CIE_FIELD)
    {
        printf("Field error\n");
        return 1;
    }
    // Calculate pixels of file
    uint32_t size_image_content, i;
    size_image_content = dib_header.width * dib_header.height;
        
    // convert pixels
    for(i = 0; i < size_image_content; i++)
    {
        xyz[i].a = rgb[i].a;
        xyz[i].y =  0.2126 * rgb[i].r +  0.7152 * rgb[i].g +  0.0722 * rgb[i].b;
        xyz[i].b = -0.1146 * rgb[i].r + -0.3854 * rgb[i].g +  0.5000 * rgb[i].b;
        xyz[i].r =  0.5000 * rgb[i].r + -0.4542 * rgb[i].g + -0.0458 * rgb[i].b; 
    }
    
    // change field
    field = CIE_FIELD;
    return 0;
}

int BMP_FILE::xyz2rgb()
{
    if(!builded)
    {
        return 1;
    }
    if(field == RGB_FIELD)
    {
        printf("Field error");
        return 1;
    }
    // Calculate pixels of file
    uint32_t size_image_content, i;
    size_image_content = dib_header.width * dib_header.height;
        
    // convert pixels
    int this_r, this_g, this_b;
    for(i = 0; i < size_image_content; i++)
    {
        rgb[i].a = xyz[i].a;
        this_r   = (int)  1.0000 * xyz[i].y + -0.0002 * xyz[i].b +  1.5748 * xyz[i].r + 0.5;
        this_g   = (int)  1.0000 * xyz[i].y + -0.1873 * xyz[i].b + -0.4681 * xyz[i].r + 0.5;
        this_b   = (int)  1.0000 * xyz[i].y +  1.8556 * xyz[i].b +  0.0001 * xyz[i].r + 0.5;
        rgb[i].r = this_r < 0 ? 0 : (this_r > 255 ? 255 : this_r); // clipping
        rgb[i].g = this_g < 0 ? 0 : (this_g > 255 ? 255 : this_g);
        rgb[i].b = this_b < 0 ? 0 : (this_b > 255 ? 255 : this_b); 
    }

    // change field
    field = RGB_FIELD;
    return 0;
}

int BMP_FILE::histogramming(float weight, BMP_FILE& output)
{
    float distribution[256] = {0};
    int accu;
    int index;
    
    // error return
    if(!builded)
    {
        printf("Build error\n");
        return 1;
    }
    if(field == RGB_FIELD)
    {
        printf("Field error\n");
        return 1;
    }

    // Calculate pixels of file
    uint32_t size_image_content, i;
    size_image_content = dib_header.width * dib_header.height;
    
    // build output
    if(output.builded)
    {
        printf("Field error\n");
        return 1;
    }
    else
    {
        output.builded      = 1;
        output.field        = field;
        output.file_header  = file_header;
        output.dib_header   = dib_header;
        // Build pixels
        output.rgb = (RGB_PIXEL*)   malloc (size_image_content * sizeof(RGB_PIXEL));
        output.xyz = (YCbCr_PIXEL*) malloc (size_image_content * sizeof(YCbCr_PIXEL));
    }

    // count distribution
    for(i = 0; i < size_image_content; i++)
    {
        index = (int) xyz[i].y + 0.5;
        distribution[index] += 1;
    }

    accu = 0;
    // create LUT
    for(i = 0; i < 256; i++)
    {
        accu += distribution[i];
        distribution[i] = accu * 255.0 / size_image_content;
    }
    
    // redistributed
    for(i = 0; i < size_image_content; i++)
    {
        index = (int) xyz[i].y + 0.5;
        output.xyz[i].y = distribution[index] * weight + xyz[i].y * (1-weight);
        output.xyz[i].b = xyz[i].b;
        output.xyz[i].r = xyz[i].r;
        output.xyz[i].a = xyz[i].a;
    }
    return 0;
}

FILTER::FILTER(int in_size, int in_seperable, int in_symmetric)
    :size(in_size), seperable(in_seperable), symmetric(in_symmetric)
{
    filter = (float*) malloc (size*size*sizeof(float));
}

FILTER::~FILTER()
{
    free(filter);
}

int BMP_FILE::convolution(FILTER& filter, BMP_FILE& output)
{
    // error return
    if(!builded)
    {
        printf("Build error\n");
        return 1;
    }
    if(field == RGB_FIELD)
    {
        printf("Field error\n");
        return 1;
    }

    // Calculate pixels of file
    uint32_t size_image_content, i, j;
    size_image_content = dib_header.width * dib_header.height;

    // build output
    if(output.builded)
    {
        printf("Field error\n");
        return 1;
    }
    else
    {
        output.builded      = 1;
        output.field        = field;
        output.file_header  = file_header;
        output.dib_header   = dib_header;
        // Build pixels
        output.rgb = (RGB_PIXEL*)   malloc (size_image_content * sizeof(RGB_PIXEL));
        output.xyz = (YCbCr_PIXEL*) malloc (size_image_content * sizeof(YCbCr_PIXEL));
    }

    // convelution
    int filter_i, filter_j;
    int index_i, index_j;
    float this_pixel_value;
    if(filter.seperable) // seperable convelution
    {
        // seperable operation;
    }
    else
    {
        for(i = 0; i < dib_header.height; i++)
        {
            for(j = 0; j < dib_header.width; j++)
            {
                this_pixel_value = 0;
                for(filter_i = 0; filter_i < filter.size; filter_i++)
                {
                    for(filter_j = 0; filter_j < filter.size; filter_j++)
                    {
                        index_i = i - filter_i + (filter.size-1)/2;
                        index_j = j - filter_j + (filter.size-1)/2;
                        index_i = index_i < 0 ? 0 : index_i < dib_header.height ? index_i : dib_header.height-1;
                        index_j = index_j < 0 ? 0 : index_j < dib_header.width  ? index_j : dib_header.width -1;
                        this_pixel_value += filter.filter[filter_i*filter.size+filter_j] * xyz[index_i*dib_header.width+index_j].y; 
                    }
                }
                output.xyz[i*dib_header.width+j].y = this_pixel_value;
                output.xyz[i*dib_header.width+j].b = xyz[i*dib_header.width+j].b;
                output.xyz[i*dib_header.width+j].r = xyz[i*dib_header.width+j].r;
                output.xyz[i*dib_header.width+j].a = xyz[i*dib_header.width+j].a;
            }
        }
    }
    return 0;
}

inline float median(float* array, int size)
{
	std::sort(array, array+size);
	return array[(size-1)/2];
}

int BMP_FILE::medianfilter(int size, BMP_FILE& output)
{
	 // error return
    if(!builded)
    {
        printf("Build error\n");
        return 1;
    }
    if(field == RGB_FIELD)
    {
        printf("Field error\n");
        return 1;
    }

    // Calculate pixels of file
    uint32_t size_image_content, i, j;
    size_image_content = dib_header.width * dib_header.height;

    // build output
    if(output.builded)
    {
        printf("Field error\n");
        return 1;
    }
    else
    {
        output.builded      = 1;
        output.field        = field;
        output.file_header  = file_header;
        output.dib_header   = dib_header;
        // Build pixels
        output.rgb = (RGB_PIXEL*)   malloc (size_image_content * sizeof(RGB_PIXEL));
        output.xyz = (YCbCr_PIXEL*) malloc (size_image_content * sizeof(YCbCr_PIXEL));
    }
    
    // Find the median of each point
    int filter_i, filter_j;
    int index_i, index_j;
    float* surround;
    surround = (float*) malloc (size*size*sizeof(float));
    for(i = 0; i < dib_header.height; i++)
    {
        for(j = 0; j < dib_header.width; j++)
        {
            for(filter_i = 0; filter_i < size; filter_i++)
            {
                for(filter_j = 0; filter_j < size; filter_j++)
                {
                    index_i = i - filter_i + (size-1)/2;
                    index_j = j - filter_j + (size-1)/2;
                    index_i = index_i < 0 ? 0 : index_i < dib_header.height ? index_i : dib_header.height-1;
                    index_j = index_j < 0 ? 0 : index_j < dib_header.width  ? index_j : dib_header.width -1;
                    surround[filter_i*size+filter_j] = xyz[index_i*dib_header.width+index_j].y;
                }
            }
            output.xyz[i*dib_header.width+j].y = median(surround, size*size);
            output.xyz[i*dib_header.width+j].b = xyz[i*dib_header.width+j].b;
            output.xyz[i*dib_header.width+j].r = xyz[i*dib_header.width+j].r;
            output.xyz[i*dib_header.width+j].a = xyz[i*dib_header.width+j].a;
        }
    }
    free(surround);
    return 0;
}

int BMP_FILE::medianfilter_convolution(int size, FILTER& filter, BMP_FILE& output)
{
	 // error return
    if(!builded)
    {
        printf("Build error\n");
        return 1;
    }
    if(field == RGB_FIELD)
    {
        printf("Field error\n");
        return 1;
    }

    // Calculate pixels of file
    uint32_t size_image_content, i, j;
    size_image_content = dib_header.width * dib_header.height;

    // build output
    if(output.builded)
    {
        printf("Field error\n");
        return 1;
    }
    else
    {
        output.builded      = 1;
        output.field        = field;
        output.file_header  = file_header;
        output.dib_header   = dib_header;
        // Build pixels
        output.rgb = (RGB_PIXEL*)   malloc (size_image_content * sizeof(RGB_PIXEL));
        output.xyz = (YCbCr_PIXEL*) malloc (size_image_content * sizeof(YCbCr_PIXEL));
    }
    
    // Find the median of each point
    int filter_i, filter_j;
    int index_i, index_j;
    float* surround, this_pixel_value_b, this_pixel_value_r;
    surround = (float*) malloc (size*size*sizeof(float));
    for(i = 0; i < dib_header.height; i++)
    {
        for(j = 0; j < dib_header.width; j++)
        {
            for(filter_i = 0; filter_i < size; filter_i++)
            {
                for(filter_j = 0; filter_j < size; filter_j++)
                {
                    index_i = i - filter_i + (size-1)/2;
                    index_j = j - filter_j + (size-1)/2;
                    index_i = index_i < 0 ? 0 : index_i < dib_header.height ? index_i : dib_header.height-1;
                    index_j = index_j < 0 ? 0 : index_j < dib_header.width  ? index_j : dib_header.width -1;
                    surround[filter_i*size+filter_j] = xyz[index_i*dib_header.width+index_j].y;
                }
            }
            output.xyz[i*dib_header.width+j].y = median(surround, size*size);
            this_pixel_value_b = 0;
            this_pixel_value_r = 0;
            for(filter_i = 0; filter_i < filter.size; filter_i++)
            {
                for(filter_j = 0; filter_j < filter.size; filter_j++)
                {
                    index_i = i - filter_i + (filter.size-1)/2;
                    index_j = j - filter_j + (filter.size-1)/2;
                    index_i = index_i < 0 ? 0 : index_i < dib_header.height ? index_i : dib_header.height-1;
                    index_j = index_j < 0 ? 0 : index_j < dib_header.width  ? index_j : dib_header.width -1;
                    this_pixel_value_b += filter.filter[filter_i*filter.size+filter_j] * xyz[index_i*dib_header.width+index_j].b; 
                    this_pixel_value_r += filter.filter[filter_i*filter.size+filter_j] * xyz[index_i*dib_header.width+index_j].r; 
                }
            }
            output.xyz[i*dib_header.width+j].b = this_pixel_value_b;
            output.xyz[i*dib_header.width+j].r = this_pixel_value_r;
            output.xyz[i*dib_header.width+j].a = xyz[i*dib_header.width+j].a;
        }
    }
    free(surround);
    return 0;
}

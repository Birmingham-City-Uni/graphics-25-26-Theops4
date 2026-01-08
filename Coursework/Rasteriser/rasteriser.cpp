#include <iostream>
#include <lodepng.h>

int main()
{
	std::string outputFilename = "output.png";

	const int width = 1920, height = 1080;
	const int nChannels = 4;

	// Set up an image buffer
	std::vector<uint8_t> imageBuffer(height*width*nChannels);

    // **** Replace this bit with your lovely rasteriser code ****

    // Set pixel values to Cyan
    for(int y = 0; y < height; ++y) 
		for (int x = 0; x < width; ++x) {
			int pixelIdx = x + y * width;
			imageBuffer[pixelIdx * nChannels + 0] = 0; 
			imageBuffer[pixelIdx * nChannels + 1] = 255;
			imageBuffer[pixelIdx * nChannels + 2] = 255; 
			imageBuffer[pixelIdx * nChannels + 3] = 255; 
		}

    // **** End lovely rasteriser code ****

    // Save the image
    int errorCode;
        errorCode = lodepng::encode(outputFilename, imageBuffer, width, height);
        if (errorCode) { // check the error code, in case an error occurred.
            std::cout << "lodepng error encoding image: " << lodepng_error_text(errorCode) << std::endl;
            return errorCode;
        }

    return 0;
}

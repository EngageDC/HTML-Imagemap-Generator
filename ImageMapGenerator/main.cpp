#include <SFML/Graphics.hpp>
#include <mach/mach_types.h>
#include <fstream.h>

#include "ResourcePath.hpp"

#define MORE_NEIGHBORHOOD_POINTS 8

struct Point
{
	Point(int x, int y) : X(x), Y(y) { }
    
	int X, Y;
};

int **fill(int x, int y, sf::Color fill, sf::Image* img)
{
    int w = img->getSize().x, h = img->getSize().y;
    
    // Initialize 2d array with filled pixels
    int** filledPixels = new int*[w];
    for(int i = 0; i < w; ++i)
        filledPixels[i] = new int[h];
    
    // Set all pixels to zero
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            filledPixels[x][y] = 0;
    
    // Determine if the position is within the actual image
	if (x < 0 || x > w-1 || y < 0 || y > h-1)
		return filledPixels;
    
    // Set reference color
	sf::Color referenceColor = img->getPixel(x, y);
	std::vector<Point> points;
	points.push_back(Point(x, y));
    
    // Perform the flood fill algorithm (http://en.wikipedia.org/wiki/Flood_fill)
	while (points.size() > 0)
	{
		Point p = points.back();
		int tx = p.X, ty = p.Y;
		points.pop_back();
        
        // Is the point within boundaries?
		if (ty < 0 || ty > h-1 || tx < 0 || tx > w-1)
			continue;
        
		sf::Color col = img->getPixel(tx, ty);
		if (col == referenceColor)
		{
            // Set pixel as being in the area
            filledPixels[tx][ty] = 1;
			img->setPixel(tx, ty, fill);
            
            // Add neighbour pixels as possible candidates
			points.push_back(Point(tx+1, ty));
			points.push_back(Point(tx-1, ty));
			points.push_back(Point(tx, ty+1));
			points.push_back(Point(tx, ty-1));
		}
	}
    
    return filledPixels;
}

std::vector<Point> findContour(sf::Image *img, int **filledPixels)
{
    int w = img->getSize().x, h = img->getSize().y;
    std::vector<Point> borderPoints;
    
    //
    // Perform the Moore neighborhood algorithm
    //
    
    // Define moore neigborhood points
    Point mooreNeigborhoodPoints[MORE_NEIGHBORHOOD_POINTS] = {
        Point(0,1),
        Point(-1,1),
        Point(-1,0),
        Point(-1,-1),
        Point(0,-1),
        Point(1,-1),
        Point(1,0),
        Point(1,1),
    };
    
    // Find starting point
    Point startPoint = Point(-1, -1);
    Point backtrackPoint = Point(0, 0);
    for (int y = h - 1; y > 0; y--)
    {
        for (int x = 0; x < w; x++)
        {
            if (filledPixels[x][y] == 1)
            {
                startPoint.X = x;
                startPoint.Y = y;
                break;
            }
            else
            {
                backtrackPoint.X = x;
                backtrackPoint.Y = y;
            }
        }
        
        if (startPoint.X != -1 && startPoint.Y != -1)
            break;
    }
    
    // Insert the starting point into the border points list
    borderPoints.push_back(startPoint);
    
    // Set our boundary point
    Point boundaryPoint = startPoint;
    
    bool first = true;
    
    //printf("Boundary point: %d,%d\n", boundaryPoint.X, boundaryPoint.Y);
    while (!(boundaryPoint.X == startPoint.X && boundaryPoint.Y == startPoint.Y) || first)
    {
        first = false;
        
        // Find out what neighboorhood point the backtrackpoint is equal to
        int startBacktrackPointIndex = -1;
        for (int i = 0; i < MORE_NEIGHBORHOOD_POINTS; i++)
        {
            Point c = Point(
                boundaryPoint.X + mooreNeigborhoodPoints[i].X,
                boundaryPoint.Y + mooreNeigborhoodPoints[i].Y
            );

            if (c.X == backtrackPoint.X && c.Y == backtrackPoint.Y)
            {
                startBacktrackPointIndex = i;
                break;
            }
        }
        
        int endBacktrackPointIndex = startBacktrackPointIndex - 1;
        if (endBacktrackPointIndex < 0)
        {
            endBacktrackPointIndex = MORE_NEIGHBORHOOD_POINTS - 1;
        }
        
        //printf("Backtrack loop: %d - %d\n", startBacktrackPointIndex, endBacktrackPointIndex);
        
        // Move counter clockwise around the boundary point, starting at the backtrack point
        Point previousPoint = Point(-1, -1);
        for (int i = startBacktrackPointIndex; i != endBacktrackPointIndex; i++)
        {
            //printf("Back track index: %d\n", i);
            // Reset index
            if (i >= MORE_NEIGHBORHOOD_POINTS)
            {
                i = 0;
            }
            
            Point c = Point(
                            boundaryPoint.X + mooreNeigborhoodPoints[i].X,
                            boundaryPoint.Y + mooreNeigborhoodPoints[i].Y
            );
            
            // Did we find a filled pixel?
            //printf("%d,%d\n", c.X, c.Y);
            if (filledPixels[c.X][c.Y] == 1)
            {
                // Save the point we found
                borderPoints.push_back(c);
                
                // Define our new boundary point
                boundaryPoint = c;
                
                // Set our new backtrack point
                backtrackPoint = previousPoint;
                
                // Start the loop over
                break;
            }
            else
            {
                previousPoint = c;
            }
        }
    }
    
    delete filledPixels;
    
    return borderPoints;
}

Point findNextArea(sf::Image *img, sf::Color color)
{
	int w = img->getSize().x, h = img->getSize().y;
    
    // Look if there is a point
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            if (img->getPixel(x, y) == color)
            {
                return Point(x, y);
            }
        }
    }
    
    return Point(-1, -1);
}

int main (int argc, const char * argv[])
{
    if (argc <= 1)
    {
        printf("Please specify a image to process");
        return 0;
    }
    
    std::string imageFileName = argv[1];
    std::string htmlFileName = imageFileName.substr(0, imageFileName.length() - 3) + "html";
    printf("Input: %s\nOutput: %s\n", imageFileName.c_str(), htmlFileName.c_str());
    
    sf::RenderWindow window(sf::VideoMode(1440, 900), "Image Map Generator");
    
    // HTML file to store the exported data in
    ofstream htmlFile;
    
    // Load a sprite to display
    sf::Texture texture;
    if (!texture.loadFromFile(resourcePath() + imageFileName))
    	return EXIT_FAILURE;
    sf::Sprite sprite(texture);
    
    sf::Image img = texture.copyToImage();
    texture.loadFromImage(img);
    
    // Set reference color
    sf::Color referenceColor = img.getPixel(200, 200);
    
    // Define circles
    std::vector<sf::CircleShape> circles;
    
    sf::Clock proccessClock;
    bool done = true;
    int areaNo = 1;

    while (window.isOpen())
    {
    	// Process events
    	sf::Event event;
    	while (window.pollEvent(event))
    	{
    		// Close window : exit
    		if (event.type == sf::Event::Closed)
    			window.close();
            
    		// Escape pressed : exit
    		if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
    			window.close();
            
            if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Button::Left && done)
            {
                referenceColor = img.getPixel(event.mouseButton.x, event.mouseButton.y);
                done = false;
                areaNo = 1;
                
                htmlFile.open(htmlFileName.c_str());
                
                htmlFile << "<!doctype html><html><head></head><body>" << std::endl;
                
                htmlFile << "<img src=\"" << imageFileName << "\" alt=\"\" usemap=\"#map\" />\n";
                htmlFile << "<map name=\"map\">\n";
            }
    	}

    	// Clear screen
    	window.clear();
    	
    	// Draw everything
    	window.draw(sprite);
        
        for (std::vector<sf::CircleShape>::iterator it = circles.begin(); it != circles.end(); ++it)
        {
            sf::CircleShape circle = (sf::CircleShape)*it;
            window.draw(circle);
        }

    	// Update the window
    	window.display();
        
        if (proccessClock.getElapsedTime().asSeconds() >= 0.1f && !done)
        {
            // Find next area
            Point nextArea = findNextArea(&img, referenceColor);
            
            // If an area was found
            if (nextArea.X != -1 && nextArea.Y != -1)
            {
                // Add circle with position to the circles vector
                sf::CircleShape circle = sf::CircleShape(5);
                circle.setPosition(nextArea.X, nextArea.Y);
                circle.setFillColor(sf::Color::Yellow);
                circles.push_back(circle);
                
                // Perform fill and contour search algorithm
                int **area = fill(nextArea.X, nextArea.Y, sf::Color(255, 0, 0), &img);
                std::vector<Point> borderPoints = findContour(&img, area);
                
                // We only consider shapes with a minimum number of points
                if (borderPoints.size() > 30)
                {
                    // Generate HTML <area> tag
                    int i = 0;
                    
                    htmlFile << "<area shape=\"polygon\" coords=\"";
                    
                    for (std::vector<Point>::iterator it = borderPoints.begin(); it != borderPoints.end(); ++it)
                    {
                        Point p = (Point)*it;
                        
                        if ((i % 5) == 0)
                        {
                            sf::CircleShape circle = sf::CircleShape(1);
                            circle.setPosition(p.X, p.Y);
                            circle.setFillColor(sf::Color::Blue);
                            circles.push_back(circle);
                            
                            htmlFile << p.X << "," << p.Y << ",";
                        }
                        
                        i++;
                    }
                    
                    htmlFile << "\" href=\"#Area" << areaNo << "\" />\n";
                    areaNo++;
                }
            }
            else // We found all areas! Let's output the HTML
            {
                htmlFile << "</map>\n";
                htmlFile << "</body></html>";
                
                htmlFile.close();
                
                // Copy the image to the current working dir (if needed) and open the HTML file in the default browser
                system(("cp \"" + resourcePath() + imageFileName + "\" .").c_str());
                system(("open " + htmlFileName).c_str());
                
                done = true;
            }
            
            // Remember to restart the clock
            proccessClock.restart();
        }
    }

	return EXIT_SUCCESS;
}

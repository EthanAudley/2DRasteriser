/*---------------------------------------------------------------------
*
* Copyright © 2016  Minsi Chen
* E-mail: m.chen@derby.ac.uk
*
* The source is written for the Graphics I and II modules. You are free
* to use and extend the functionality. The code provided here is functional
* however the author does not guarantee its performance.
---------------------------------------------------------------------*/
#include <algorithm>
#include <math.h>
#include <vector>

#include "Rasterizer.h"

Rasterizer::Rasterizer(void)
{
	mFramebuffer = NULL;
	mScanlineLUT = NULL;
}

void Rasterizer::ClearScanlineLUT()
{
	Scanline *pScanline = mScanlineLUT;

	for (int y = 0; y < mHeight; y++)
	{
		(pScanline + y)->clear();
		(pScanline + y)->shrink_to_fit();
	}
}

unsigned int Rasterizer::ComputeOutCode(const Vector2 & p, const ClipRect& clipRect)
{
	unsigned int CENTRE = 0x0;
	unsigned int LEFT = 0x1;
	unsigned int RIGHT = 0x1 << 1;
	unsigned int BOTTOM = 0x1 << 2;
	unsigned int TOP = 0x1 << 3;
	unsigned int outcode = CENTRE;

	if (p[0] < clipRect.left)
		outcode |= LEFT;
	else if (p[0] >= clipRect.right)
		outcode |= RIGHT;

	if (p[1] < clipRect.bottom)
		outcode |= BOTTOM;
	else if (p[1] >= clipRect.top)
		outcode |= TOP;

	return outcode;
}

bool Rasterizer::ClipLine(const Vertex2d & v1, const Vertex2d & v2, const ClipRect& clipRect, Vector2 & outP1, Vector2 & outP2)
{
	//TODO: EXTRA This is not directly prescribed as an assignment exercise. 
	//However, if you want to create an efficient and robust rasteriser, clipping is a usefull addition.
	//The following code is the starting point of the Cohen-Sutherland clipping algorithm.
	//If you complete its implementation, you can test it by calling prior to calling any DrawLine2D .

	const Vector2 p1 = v1.position;
	const Vector2 p2 = v2.position;
	unsigned int outcode1 = ComputeOutCode(p1, clipRect);
	unsigned int outcode2 = ComputeOutCode(p2, clipRect);

	outP1 = p1;
	outP2 = p2;

	bool draw = false;

	return true;
}

void Rasterizer::WriteRGBAToFramebuffer(int x, int y, const Colour4 & colour)
{
	//If y is less than the height of the frame buffer - 1 and it is greater than 0 and
	//x is less than the wieght of the frame buffer - 1 and it is greater than 0 step into the statement.
	if (y < mFramebuffer->GetHeight() - 1 && y >= 0 && x < mFramebuffer->GetWidth() - 1 && x >= 0) 
	{
		PixelRGBA *pixel = mFramebuffer->GetBuffer();
		pixel[y*mWidth + x] = colour;
	}
}

Rasterizer::Rasterizer(int width, int height)
{
	//Initialise the rasterizer to its initial state
	mFramebuffer = new Framebuffer(width, height);
	mScanlineLUT = new Scanline[height];
	mWidth = width;
	mHeight = height;

	mBGColour.SetVector(0.0, 0.0, 0.0, 1.0);	//default bg colour is black
	mFGColour.SetVector(1.0, 1.0, 1.0, 1.0);    //default fg colour is white

	mGeometryMode = LINE;
	mFillMode = UNFILLED;
	mBlendMode = NO_BLEND;

	SetClipRectangle(0, mWidth, 0, mHeight);
}

Rasterizer::~Rasterizer()
{
	delete mFramebuffer;
	delete[] mScanlineLUT;
}

void Rasterizer::Clear(const Colour4& colour)
{
	PixelRGBA *pixel = mFramebuffer->GetBuffer();

	SetBGColour(colour);

	int size = mWidth*mHeight;

	for (int i = 0; i < size; i++)
	{
		//fill all pixels in the framebuffer with background colour.
		*(pixel + i) = mBGColour;
	}
}

void Rasterizer::DrawPoint2D(const Vector2& pt, int size)
{
	int x = pt[0];
	int y = pt[1];

	WriteRGBAToFramebuffer(x, y, mFGColour);
}

void Rasterizer::DrawLine2D(const Vertex2d & v1, const Vertex2d & v2, int thickness)
{
	//Set pt1 or pt2 to v1 or v2's poisition.
	Vector2 pt1 = v1.position;
	Vector2 pt2 = v2.position;

	//Boolean variable that is set true if x greater than dx.
	bool swap_verticies = pt1[0] > pt2[0];

	if (swap_verticies)
	{
		//Sets vector point one to equal vector point two and vector point two to equal vector point one.
		pt1 = v2.position;
		pt2 = v1.position;
	}

	//Variables that are setto the points within pt1 and pt2.
	int x = pt1[0];
	int y = pt1[1];
	int ex = pt2[0];
	int ey = pt2[1];
	int dx = pt2[0] - pt1[0];
	int dy = pt2[1] - pt1[1];

	int epsilon = 0;

	//Boolean variable that is set true if the condition is met.
	bool negative_slope = dy < 0;

	if (negative_slope)
	{
		//Sets y and dy to the their negative form.
		y = -y;
		dy = -dy;
	}

	//Boolean variable that is set true if the condition is met.
	bool swap_xy = abs(dx) < abs(dy);

	//If swapxy is true then swap the variables.
	if (swap_xy)
	{
		std::swap(x, y); //Built in standard swap which will set x to equal y and y to equal x.
		std::swap(dx, dy); //dx equals dy and dy equals dx.
		std::swap(ex, ey); //ex equals ey and ey equals ex.
	}

	if (negative_slope && swap_xy)
	{
		// Sets the end point ex to the value of its negative form.
		ex = -ex;
	}

	while (x <= ex)
	{
		//So that x and y do not have to be flipped and then re flipped, i've created new local instances of x and y.
		int positionX = x;
		int positionY = y;

		Colour4 edgeColour;

		if (swap_xy)
		{
			std::swap(positionX, positionY); //Standard swap x and y.
		}

		if (negative_slope)
		{
			//Set y to its negative form.
			positionY = -positionY;
		}

		//If mFillMode is equal to INTERPOLATED_FILLED then step into the statment and perform calculations that will colour interpolate two colours,
		//else set colour to v1.colour (a single colour).
		if (mFillMode == INTERPOLATED_FILLED) //Enters the statement if interpolated colours is true else it will set the colour to a RGB value.
		{
			float t = 0;

			if (swap_xy)
			{
				t = abs(pt1[1] - positionY) / abs(pt2[1] - pt1[1]); //Calculates each point along an arbitrary line of the x axis.
			}
			else
			{
				t = abs(pt1[0] - positionX) / abs(pt2[0] - pt1[0]); //Calculates each point along an arbitrary line of the y axis.
			}

			if (swap_verticies)
			{
				edgeColour = (v1.colour * t) + (v2.colour * (1 - t)); //Sets the colour of each pixel along an arbitrary line.
			}
			else
			{

				edgeColour = (v2.colour * t) + (v1.colour * (1 - t)); //Sets the colour of each pixel along an arbitrary line.
			}

			SetFGColour(edgeColour);
		}
		else
		{
			edgeColour = v1.colour; //Set edgeColour to a RGB value.
			SetFGColour(edgeColour);
		}

		for (int i = 0; i < thickness; i++)
		{
			int currentLocation = i / 2; //Set currentLocation to i divide 2 which will half the value by 2.
			int drawLine = swap_xy ? 1 : 0; //If swap_xy is true then set drawLine to either 1 else set drawLine to 0.

			//Set the local instances to new instances of x and y.
			int lineX = positionX;
			int lineY = positionY;

			//Modules of i. The first instance will be 0 and the second will be 1. 
			//If i % 2 == drawLine is true then set currentLocation to it's negative form else set it to itself.
			i % 2 == drawLine ? currentLocation = -currentLocation : currentLocation = currentLocation;

			//If swap_xy is true then set lineX and lineY to add 1, 2, 3 ... on there axises
			swap_xy ? lineX = lineX + currentLocation : lineY = lineY + currentLocation;

			//Create a temporary Vector2 variable from lineX and lineY.
			Vector2 temp(lineX, lineY);
			DrawPoint2D(temp);
		}
		epsilon += dy;
		if ((epsilon << 1) >= dx)
		{
			y++;

			epsilon -= dx;
		}
		x++;
	}
}

void Rasterizer::DrawUnfilledPolygon2D(const Vertex2d * vertices, int count)
{
	//TODO:
	//Ex 2.1 Implement the Rasterizer::DrawUnfilledPolygon2D method so that it is capable of drawing an unfilled polygon, i.e. only the edges of a polygon are rasterised. 
	//Please note, in order to complete this exercise, you must first complete Ex1.1 since DrawLine2D method is reusable here.
	//Note: The edges of a given polygon can be found by conntecting two adjacent vertices in the vertices array.
	//Use Test 3 (Press F3) to test your solution.
	int i = 0;
	while (i < count - 1)
	{
		DrawLine2D(vertices[i], vertices[i + 1]);
		i = i++;
	}
	DrawLine2D(vertices[count - 1], vertices[0]);
}

bool myfunction(ScanlineLUTItem v1, ScanlineLUTItem v2)
{
	if (v1.pos_x < v2.pos_x)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void Rasterizer::ScanlineFillPolygon2D(const Vertex2d * vertices, int count)
{
	ClearScanlineLUT();
	//TODO:
	//Ex 2.2 Implement the Rasterizer::ScanlineFillPolygon2D method method so that it is capable of drawing a solidly filled polygon.
	//Note: You can implement floodfill for this exercise however scanline fill is considered a more efficient and robust solution.
	//		You should be able to reuse DrawUnfilledPolygon2D here.
	//
	//Use Test 4 (Press F4) to test your solution, this is a simple test case as all polygons are convex.
	//Use Test 5 (Press F5) to test your solution, this is a complex test case with one non-convex polygon.

	//Ex 2.3 Extend Rasterizer::ScanlineFillPolygon2D method so that it is capable of alpha blending, i.e. draw translucent polygons.
	//Note: The variable mBlendMode indicates if the blend mode is set to alpha blending.
	//To do alpha blending during filling, the new colour of a point should be combined with the existing colour in the framebuffer using the alpha value.
	//Use Test 6 (Press F6) to test your solution

	int heightY = mHeight;
	int y = 0;

	ScanlineLUTItem scanItem;

	Colour4 colour = vertices[0].colour;

	//While y is less than mHeight - 1 loop until y is greater than mHeight.
	while (y < mHeight - 1)
	{
		int edge = 0; //Represents the amount of edges a polygon has.
		while (edge < count)
		{
			//Set x and y to a posistion on the current polygon.
			int verticeY = vertices[edge].position[1];
			int verticeX = vertices[edge].position[0];
			//Set x2 and y2 to a poisition on the current polygon.
			int verticeX2 = vertices[edge + 1].position[0];
			int verticeY2 = vertices[edge + 1].position[1];
	
			if (edge > count)
			{
				int verticeX2 = vertices[0].position[0];
				int verticeY2 = vertices[0].position[1];
			}

			if (verticeY - verticeY2 != 0)
			{
				//Check to see if the scanline and edge intersects.
				if ((verticeY <= y && verticeY2 >= y) || (verticeY >= y && verticeY2 <= y))
				{
					//Set the value of x,
					//then set scanitem to a colour with the current x position.
					//Finally add the item to the LUT.
					float dx = (float)(verticeX - verticeX2) / (verticeY - verticeY2);
					int x = verticeX + (y - verticeY) * dx;
					scanItem = { colour, x };
					mScanlineLUT[y].push_back(scanItem);
				}
			}
			edge++;
		}
		y++;
	}

	//iterate until y is greater than mHeight - 1 (y = 720 while mHeight is qeual to 719).
	for (int y = 0; y < mHeight - 1; y++)
	{
		int y2 = 0;
		//Get the size of the current LUT and check to see if it contains a value.
		if (mScanlineLUT[y].size() > 1)
		{
			//Sort LUT in order of increasing x positions.
			std::sort(mScanlineLUT[y].begin(), mScanlineLUT[y].end(), myfunction);

			//Iiterate until i is greater mScanLineLUT
			for (int i = 0; i < mScanlineLUT[y].size() - 1; i++)
			{
				//Initilize pt1 and pt2 to a position of x and y.
				Vector2 pt1(mScanlineLUT[y][i].pos_x, y);
				Vector2 pt2(mScanlineLUT[y][i + 1].pos_x, y);

				//Initilize vertex2d v1 and v2 to colour and a vector2 position.
				Vertex2d v1 = { mScanlineLUT[y][i].colour, pt1 };
				Vertex2d v2 = { mScanlineLUT[y][i + 1].colour, pt2 };
				DrawLine2D(v1, v2);
			}
			y2++;
		}
	}
}

void Rasterizer::ScanlineInterpolatedFillPolygon2D(const Vertex2d * vertices, int count)
{
	//TODO:
	//Ex 2.4 Implement Rasterizer::ScanlineInterpolatedFillPolygon2D method so that it is capable of performing interpolated filling.
	//Note: mFillMode is set to INTERPOLATED_FILL
	//		This exercise will be more straightfoward if Ex 1.3 has been implemented in DrawLine2D
	//Use Test 7 to test your solution

	ScanlineFillPolygon2D(vertices, count);
	
}

void Rasterizer::DrawCircle2D(const Circle2D & inCircle, bool filled)
{
	//TODO:
	//Ex 2.5 Implement Rasterizer::DrawCircle2D method so that it can draw a filled circle.
	//Note: For a simple solution, you can first attempt to draw an unfilled circle in the same way as drawing an unfilled polygon.
	//Use Test 8 to test your solution

	//Colour4 colour = inCircle.colour;

	float PI = 3.1415926;
	float nsegment = 40;
	float t = 0;
	float dt = (2 * PI) / nsegment;

	Vertex2d pt1;
	Vertex2d pt2;

	pt1.position[0] = inCircle.centre[0] + inCircle.radius;
	pt1.position[1] = inCircle.centre[1];

	while (t <= 2 * PI)
	{
		t += dt;

		pt2.position[0] = inCircle.radius * cos(t) + inCircle.centre[0];
		pt2.position[1] = inCircle.radius * sin(t) + inCircle.centre[1];

		pt1.colour = inCircle.colour;

		DrawLine2D(pt1, pt2);

		pt1 = pt2;
	}
}

Framebuffer *Rasterizer::GetFrameBuffer() const
{
	return mFramebuffer;
}

﻿#pragma once
///////////////////////////////////////////////////////////////////////
//	    C Implementation of Wu's Color Quantizer (v. 2)
//	    (see Graphics Gems vol. II, pp. 126-133)
//
// Author:	Xiaolin Wu
// Dept. of Computer Science
// Univ. of Western Ontario
// London, Ontario N6A 5B7
// wu@csd.uwo.ca
//
// Copyright(c) 2018 - 2021 Miller Cy Chan
// 
// Algorithm: Greedy orthogonal bipartition of RGB space for variance
// 	   minimization aided by inclusion-exclusion tricks.
// 	   For speed no nearest neighbor search is done. Slightly
// 	   better performance can be expected by more sophisticated
// 	   but more expensive versions.
// 
// Free to distribute, comments and suggestions are appreciated.
///////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "WuQuantizer.h"
#include "bitmapUtilities.h"
#include <unordered_map>

namespace nQuant
{
	/// <summary><para>Shift color values right this many bits.</para><para>This reduces the granularity of the color maps produced, making it much faster.</para></summary>
	/// 3 = value error of 8 (0 and 7 will look the same to it, 0 and 8 different); Takes ~4MB for color tables; ~.25 -> .50 seconds
	/// 2 = value error of 4; Takes ~64MB for color tables; ~3 seconds
	/// RAM usage roughly estimated with: ( ( 256 >> SidePixShift ) ^ 4 ) * 60
	/// Default SidePixShift = 3
	const BYTE SIDEPIXSHIFT = 3;
	const BYTE MAXSIDEINDEX = 256 / (1 << SIDEPIXSHIFT);
	const BYTE SIDESIZE = MAXSIDEINDEX + 1;
	const UINT TOTAL_SIDESIZE = SIDESIZE * SIDESIZE * SIDESIZE * SIDESIZE;

	bool hasSemiTransparency = false;
	int m_transparentPixelIndex = -1;
	ARGB m_transparentColor = Color::Transparent;
	double PR = .299, PG = .587, PB = .114;
	unordered_map<ARGB, vector<unsigned short> > closestMap;
	unordered_map<ARGB, unsigned short> nearestMap;

	struct Box {
		BYTE AlphaMinimum = 0;
		BYTE AlphaMaximum = 0;
		BYTE RedMinimum = 0;
		BYTE RedMaximum = 0;
		BYTE GreenMinimum = 0;
		BYTE GreenMaximum = 0;
		BYTE BlueMinimum = 0;
		BYTE BlueMaximum = 0;
		UINT Size = 0;
	};

	struct CubeCut {
		bool valid;
		BYTE position;
		float value;

		CubeCut(bool isValid, BYTE cutPoint, float result) {
			valid = isValid;
			position = cutPoint;
			value = result;
		}
	};

	struct ColorData {
		unique_ptr<long[]> weights;
		unique_ptr<long[]> momentsAlpha;
		unique_ptr<long[]> momentsRed;
		unique_ptr<long[]> momentsGreen;
		unique_ptr<long[]> momentsBlue;
		unique_ptr<float[]> moments;

		unique_ptr<ARGB[]> pixels;

		UINT pixelsCount = 0;
		UINT pixelFillingCounter = 0;

		ColorData(UINT sideSize, UINT bitmapWidth, UINT bitmapHeight) {
			const int TOTAL_SIDESIZE = sideSize * sideSize * sideSize * sideSize;
			weights = make_unique<long[]>(TOTAL_SIDESIZE);
			momentsAlpha = make_unique<long[]>(TOTAL_SIDESIZE);
			momentsRed = make_unique<long[]>(TOTAL_SIDESIZE);
			momentsGreen = make_unique<long[]>(TOTAL_SIDESIZE);
			momentsBlue = make_unique<long[]>(TOTAL_SIDESIZE);
			moments = make_unique<float[]>(TOTAL_SIDESIZE);
			pixelsCount = bitmapWidth * bitmapHeight;
			pixels = make_unique<ARGB[]>(pixelsCount);
		}

		inline ARGB* GetPixels() {
			return pixels.get();
		}

		inline void AddPixel(ARGB pixel)
		{
			pixels[pixelFillingCounter] = pixel;
			++pixelFillingCounter;
		}
	};

	inline UINT Index(BYTE red, BYTE green, BYTE blue) {
		return red + green * SIDESIZE + blue * SIDESIZE * SIDESIZE;
	}

	inline UINT Index(BYTE alpha, BYTE red, BYTE green, BYTE blue) {
		return alpha + red * SIDESIZE + green * SIDESIZE * SIDESIZE + blue * SIDESIZE * SIDESIZE * SIDESIZE;
	}

	inline float Volume(const Box& cube, long* moment)
	{
		return (moment[Index(cube.AlphaMaximum, cube.RedMaximum, cube.GreenMaximum, cube.BlueMaximum)] -
			moment[Index(cube.AlphaMaximum, cube.RedMaximum, cube.GreenMinimum, cube.BlueMaximum)] -
			moment[Index(cube.AlphaMaximum, cube.RedMinimum, cube.GreenMaximum, cube.BlueMaximum)] +
			moment[Index(cube.AlphaMaximum, cube.RedMinimum, cube.GreenMinimum, cube.BlueMaximum)] -
			moment[Index(cube.AlphaMinimum, cube.RedMaximum, cube.GreenMaximum, cube.BlueMaximum)] +
			moment[Index(cube.AlphaMinimum, cube.RedMaximum, cube.GreenMinimum, cube.BlueMaximum)] +
			moment[Index(cube.AlphaMinimum, cube.RedMinimum, cube.GreenMaximum, cube.BlueMaximum)] -
			moment[Index(cube.AlphaMinimum, cube.RedMinimum, cube.GreenMinimum, cube.BlueMaximum)]) -
			(moment[Index(cube.AlphaMaximum, cube.RedMaximum, cube.GreenMaximum, cube.BlueMinimum)] -
				moment[Index(cube.AlphaMinimum, cube.RedMaximum, cube.GreenMaximum, cube.BlueMinimum)] -
				moment[Index(cube.AlphaMaximum, cube.RedMaximum, cube.GreenMinimum, cube.BlueMinimum)] +
				moment[Index(cube.AlphaMinimum, cube.RedMaximum, cube.GreenMinimum, cube.BlueMinimum)] -
				moment[Index(cube.AlphaMaximum, cube.RedMinimum, cube.GreenMaximum, cube.BlueMinimum)] +
				moment[Index(cube.AlphaMinimum, cube.RedMinimum, cube.GreenMaximum, cube.BlueMinimum)] +
				moment[Index(cube.AlphaMaximum, cube.RedMinimum, cube.GreenMinimum, cube.BlueMinimum)] -
				moment[Index(cube.AlphaMinimum, cube.RedMinimum, cube.GreenMinimum, cube.BlueMinimum)]);
	}

	inline float Volume(const Box& cube, float* moment)
	{
		return (moment[Index(cube.AlphaMaximum, cube.RedMaximum, cube.GreenMaximum, cube.BlueMaximum)] -
			moment[Index(cube.AlphaMaximum, cube.RedMaximum, cube.GreenMinimum, cube.BlueMaximum)] -
			moment[Index(cube.AlphaMaximum, cube.RedMinimum, cube.GreenMaximum, cube.BlueMaximum)] +
			moment[Index(cube.AlphaMaximum, cube.RedMinimum, cube.GreenMinimum, cube.BlueMaximum)] -
			moment[Index(cube.AlphaMinimum, cube.RedMaximum, cube.GreenMaximum, cube.BlueMaximum)] +
			moment[Index(cube.AlphaMinimum, cube.RedMaximum, cube.GreenMinimum, cube.BlueMaximum)] +
			moment[Index(cube.AlphaMinimum, cube.RedMinimum, cube.GreenMaximum, cube.BlueMaximum)] -
			moment[Index(cube.AlphaMinimum, cube.RedMinimum, cube.GreenMinimum, cube.BlueMaximum)]) -
			(moment[Index(cube.AlphaMaximum, cube.RedMaximum, cube.GreenMaximum, cube.BlueMinimum)] -
				moment[Index(cube.AlphaMinimum, cube.RedMaximum, cube.GreenMaximum, cube.BlueMinimum)] -
				moment[Index(cube.AlphaMaximum, cube.RedMaximum, cube.GreenMinimum, cube.BlueMinimum)] +
				moment[Index(cube.AlphaMinimum, cube.RedMaximum, cube.GreenMinimum, cube.BlueMinimum)] -
				moment[Index(cube.AlphaMaximum, cube.RedMinimum, cube.GreenMaximum, cube.BlueMinimum)] +
				moment[Index(cube.AlphaMinimum, cube.RedMinimum, cube.GreenMaximum, cube.BlueMinimum)] +
				moment[Index(cube.AlphaMaximum, cube.RedMinimum, cube.GreenMinimum, cube.BlueMinimum)] -
				moment[Index(cube.AlphaMinimum, cube.RedMinimum, cube.GreenMinimum, cube.BlueMinimum)]);
	}

	inline float Top(const Box& cube, Pixel direction, BYTE position, long* moment)
	{
		switch (direction)
		{
		case Alpha:
			return (moment[Index(position, cube.RedMaximum, cube.GreenMaximum, cube.BlueMaximum)] -
				moment[Index(position, cube.RedMaximum, cube.GreenMinimum, cube.BlueMaximum)] -
				moment[Index(position, cube.RedMinimum, cube.GreenMaximum, cube.BlueMaximum)] +
				moment[Index(position, cube.RedMinimum, cube.GreenMinimum, cube.BlueMaximum)]) -
				(moment[Index(position, cube.RedMaximum, cube.GreenMaximum, cube.BlueMinimum)] -
					moment[Index(position, cube.RedMaximum, cube.GreenMinimum, cube.BlueMinimum)] -
					moment[Index(position, cube.RedMinimum, cube.GreenMaximum, cube.BlueMinimum)] +
					moment[Index(position, cube.RedMinimum, cube.GreenMinimum, cube.BlueMinimum)]);

		case Red:
			return (moment[Index(cube.AlphaMaximum, position, cube.GreenMaximum, cube.BlueMaximum)] -
				moment[Index(cube.AlphaMaximum, position, cube.GreenMinimum, cube.BlueMaximum)] -
				moment[Index(cube.AlphaMinimum, position, cube.GreenMaximum, cube.BlueMaximum)] +
				moment[Index(cube.AlphaMinimum, position, cube.GreenMinimum, cube.BlueMaximum)]) -
				(moment[Index(cube.AlphaMaximum, position, cube.GreenMaximum, cube.BlueMinimum)] -
					moment[Index(cube.AlphaMaximum, position, cube.GreenMinimum, cube.BlueMinimum)] -
					moment[Index(cube.AlphaMinimum, position, cube.GreenMaximum, cube.BlueMinimum)] +
					moment[Index(cube.AlphaMinimum, position, cube.GreenMinimum, cube.BlueMinimum)]);

		case Green:
			return (moment[Index(cube.AlphaMaximum, cube.RedMaximum, position, cube.BlueMaximum)] -
				moment[Index(cube.AlphaMaximum, cube.RedMinimum, position, cube.BlueMaximum)] -
				moment[Index(cube.AlphaMinimum, cube.RedMaximum, position, cube.BlueMaximum)] +
				moment[Index(cube.AlphaMinimum, cube.RedMinimum, position, cube.BlueMaximum)]) -
				(moment[Index(cube.AlphaMaximum, cube.RedMaximum, position, cube.BlueMinimum)] -
					moment[Index(cube.AlphaMaximum, cube.RedMinimum, position, cube.BlueMinimum)] -
					moment[Index(cube.AlphaMinimum, cube.RedMaximum, position, cube.BlueMinimum)] +
					moment[Index(cube.AlphaMinimum, cube.RedMinimum, position, cube.BlueMinimum)]);

		case Blue:
			return (moment[Index(cube.AlphaMaximum, cube.RedMaximum, cube.GreenMaximum, position)] -
				moment[Index(cube.AlphaMaximum, cube.RedMaximum, cube.GreenMinimum, position)] -
				moment[Index(cube.AlphaMaximum, cube.RedMinimum, cube.GreenMaximum, position)] +
				moment[Index(cube.AlphaMaximum, cube.RedMinimum, cube.GreenMinimum, position)]) -
				(moment[Index(cube.AlphaMinimum, cube.RedMaximum, cube.GreenMaximum, position)] -
					moment[Index(cube.AlphaMinimum, cube.RedMaximum, cube.GreenMinimum, position)] -
					moment[Index(cube.AlphaMinimum, cube.RedMinimum, cube.GreenMaximum, position)] +
					moment[Index(cube.AlphaMinimum, cube.RedMinimum, cube.GreenMinimum, position)]);

		default:
			return 0;
		}
	}

	inline float Bottom(const Box& cube, Pixel direction, long* moment)
	{
		switch (direction)
		{
		case Alpha:
			return (-moment[Index(cube.AlphaMinimum, cube.RedMaximum, cube.GreenMaximum, cube.BlueMaximum)] +
				moment[Index(cube.AlphaMinimum, cube.RedMaximum, cube.GreenMinimum, cube.BlueMaximum)] +
				moment[Index(cube.AlphaMinimum, cube.RedMinimum, cube.GreenMaximum, cube.BlueMaximum)] -
				moment[Index(cube.AlphaMinimum, cube.RedMinimum, cube.GreenMinimum, cube.BlueMaximum)]) -
				(-moment[Index(cube.AlphaMinimum, cube.RedMaximum, cube.GreenMaximum, cube.BlueMinimum)] +
					moment[Index(cube.AlphaMinimum, cube.RedMaximum, cube.GreenMinimum, cube.BlueMinimum)] +
					moment[Index(cube.AlphaMinimum, cube.RedMinimum, cube.GreenMaximum, cube.BlueMinimum)] -
					moment[Index(cube.AlphaMinimum, cube.RedMinimum, cube.GreenMinimum, cube.BlueMinimum)]);

		case Red:
			return (-moment[Index(cube.AlphaMaximum, cube.RedMinimum, cube.GreenMaximum, cube.BlueMaximum)] +
				moment[Index(cube.AlphaMaximum, cube.RedMinimum, cube.GreenMinimum, cube.BlueMaximum)] +
				moment[Index(cube.AlphaMinimum, cube.RedMinimum, cube.GreenMaximum, cube.BlueMaximum)] -
				moment[Index(cube.AlphaMinimum, cube.RedMinimum, cube.GreenMinimum, cube.BlueMaximum)]) -
				(-moment[Index(cube.AlphaMaximum, cube.RedMinimum, cube.GreenMaximum, cube.BlueMinimum)] +
					moment[Index(cube.AlphaMaximum, cube.RedMinimum, cube.GreenMinimum, cube.BlueMinimum)] +
					moment[Index(cube.AlphaMinimum, cube.RedMinimum, cube.GreenMaximum, cube.BlueMinimum)] -
					moment[Index(cube.AlphaMinimum, cube.RedMinimum, cube.GreenMinimum, cube.BlueMinimum)]);

		case Green:
			return (-moment[Index(cube.AlphaMaximum, cube.RedMaximum, cube.GreenMinimum, cube.BlueMaximum)] +
				moment[Index(cube.AlphaMaximum, cube.RedMinimum, cube.GreenMinimum, cube.BlueMaximum)] +
				moment[Index(cube.AlphaMinimum, cube.RedMaximum, cube.GreenMinimum, cube.BlueMaximum)] -
				moment[Index(cube.AlphaMinimum, cube.RedMinimum, cube.GreenMinimum, cube.BlueMaximum)]) -
				(-moment[Index(cube.AlphaMaximum, cube.RedMaximum, cube.GreenMinimum, cube.BlueMinimum)] +
					moment[Index(cube.AlphaMaximum, cube.RedMinimum, cube.GreenMinimum, cube.BlueMinimum)] +
					moment[Index(cube.AlphaMinimum, cube.RedMaximum, cube.GreenMinimum, cube.BlueMinimum)] -
					moment[Index(cube.AlphaMinimum, cube.RedMinimum, cube.GreenMinimum, cube.BlueMinimum)]);

		case Blue:
			return (-moment[Index(cube.AlphaMaximum, cube.RedMaximum, cube.GreenMaximum, cube.BlueMinimum)] +
				moment[Index(cube.AlphaMaximum, cube.RedMaximum, cube.GreenMinimum, cube.BlueMinimum)] +
				moment[Index(cube.AlphaMaximum, cube.RedMinimum, cube.GreenMaximum, cube.BlueMinimum)] -
				moment[Index(cube.AlphaMaximum, cube.RedMinimum, cube.GreenMinimum, cube.BlueMinimum)]) -
				(-moment[Index(cube.AlphaMinimum, cube.RedMaximum, cube.GreenMaximum, cube.BlueMinimum)] +
					moment[Index(cube.AlphaMinimum, cube.RedMaximum, cube.GreenMinimum, cube.BlueMinimum)] +
					moment[Index(cube.AlphaMinimum, cube.RedMinimum, cube.GreenMaximum, cube.BlueMinimum)] -
					moment[Index(cube.AlphaMinimum, cube.RedMinimum, cube.GreenMinimum, cube.BlueMinimum)]);

		default:
			return 0;
		}
	}

	void CompileColorData(ColorData& colorData, const Color& color, const BYTE alphaThreshold, const BYTE alphaFader)
	{
		BYTE pixelBlue = color.GetB();
		BYTE pixelGreen = color.GetG();
		BYTE pixelRed = color.GetR();
		BYTE pixelAlpha = color.GetA();

		BYTE indexAlpha = static_cast<BYTE>((pixelAlpha >> SIDEPIXSHIFT) + 1);
		BYTE indexRed = static_cast<BYTE>((pixelRed >> SIDEPIXSHIFT) + 1);
		BYTE indexGreen = static_cast<BYTE>((pixelGreen >> SIDEPIXSHIFT) + 1);
		BYTE indexBlue = static_cast<BYTE>((pixelBlue >> SIDEPIXSHIFT) + 1);

		if (pixelAlpha > alphaThreshold) {
			if (pixelAlpha < BYTE_MAX) {
				short alpha = pixelAlpha + (pixelAlpha % alphaFader);
				pixelAlpha = static_cast<BYTE>(alpha > BYTE_MAX ? BYTE_MAX : alpha);
				indexAlpha = static_cast<BYTE>((pixelAlpha >> 3) + 1);
			}

			const int index = Index(indexAlpha, indexRed, indexGreen, indexBlue);
			if (index < TOTAL_SIDESIZE) {
				colorData.weights[index]++;
				colorData.momentsRed[index] += pixelRed;
				colorData.momentsGreen[index] += pixelGreen;
				colorData.momentsBlue[index] += pixelBlue;
				colorData.momentsAlpha[index] += pixelAlpha;
				colorData.moments[index] += sqr(pixelAlpha) + sqr(pixelRed) + sqr(pixelGreen) + sqr(pixelBlue);
			}
		}

		colorData.AddPixel(Color::MakeARGB(pixelAlpha, pixelRed, pixelGreen, pixelBlue));
	}

	void BuildHistogram(ColorData& colorData, Bitmap* sourceImage, BYTE alphaThreshold, BYTE alphaFader)
	{
		const UINT bitDepth = GetPixelFormatSize(sourceImage->GetPixelFormat());
		const UINT bitmapWidth = sourceImage->GetWidth();
		const UINT bitmapHeight = sourceImage->GetHeight();

		int pixelIndex = 0;
		if (bitDepth <= 16) {
			for (UINT y = 0; y < bitmapHeight; ++y) {
				for (UINT x = 0; x < bitmapWidth; ++x, ++pixelIndex) {
					Color color;
					sourceImage->GetPixel(x, y, &color);
					if (color.GetA() < BYTE_MAX) {						
						if (color.GetA() == 0) {
							m_transparentPixelIndex = pixelIndex;
							m_transparentColor = color.GetValue();
						}
						else
							hasSemiTransparency = true;
					}
					CompileColorData(colorData, color, alphaThreshold, alphaFader);
				}
			}
			return;
		}

		BitmapData data;
		Status status = sourceImage->LockBits(&Rect(0, 0, bitmapWidth, bitmapHeight), ImageLockModeRead, sourceImage->GetPixelFormat(), &data);
		if (status != Ok)
			return;

		auto pRowSource = (LPBYTE)data.Scan0;
		UINT strideSource;

		if (data.Stride > 0) strideSource = data.Stride;
		else
		{
			// Compensate for possible negative stride
			// (not needed for first loop, but we have to do it
			// for second loop anyway)
			pRowSource += bitmapHeight * data.Stride;
			strideSource = -data.Stride;
		}

		// First loop: gather color information
		for (UINT y = 0; y < bitmapHeight; ++y)	// For each row...
		{
			auto pPixelSource = pRowSource;

			for (UINT x = 0; x < bitmapWidth; ++x, ++pixelIndex)	// ...for each pixel...
			{
				BYTE pixelBlue = *pPixelSource++;
				BYTE pixelGreen = *pPixelSource++;
				BYTE pixelRed = *pPixelSource++;
				BYTE pixelAlpha = bitDepth < 32 ? BYTE_MAX : *pPixelSource++;

				Color color(Color::MakeARGB(pixelAlpha, pixelRed, pixelGreen, pixelBlue));
				if (pixelAlpha < BYTE_MAX) {
					if (pixelAlpha == 0) {
						m_transparentPixelIndex = pixelIndex;
						m_transparentColor = color.GetValue();
					}
					else
						hasSemiTransparency = true;
				}
				CompileColorData(colorData, color, alphaThreshold, alphaFader);
			}

			pRowSource += strideSource;
		}

		sourceImage->UnlockBits(&data);
	}

	void CalculateMoments(ColorData& data)
	{
		const UINT SIDESIZE_3 = SIDESIZE * SIDESIZE * SIDESIZE;
		for (BYTE alphaIndex = 1; alphaIndex <= MAXSIDEINDEX; ++alphaIndex)
		{
			auto xarea = make_unique<UINT[]>(SIDESIZE_3);
			auto xareaAlpha = make_unique<UINT[]>(SIDESIZE_3);
			auto xareaRed = make_unique<UINT[]>(SIDESIZE_3);
			auto xareaGreen = make_unique<UINT[]>(SIDESIZE_3);
			auto xareaBlue = make_unique<UINT[]>(SIDESIZE_3);
			auto xarea2 = make_unique<float[]>(SIDESIZE_3);

			for (BYTE redIndex = 1; redIndex <= MAXSIDEINDEX; ++redIndex)
			{
				UINT area[SIDESIZE] = { 0 };
				UINT areaAlpha[SIDESIZE] = { 0 };
				UINT areaRed[SIDESIZE] = { 0 };
				UINT areaGreen[SIDESIZE] = { 0 };
				UINT areaBlue[SIDESIZE] = { 0 };
				float area2[SIDESIZE] = { 0 };

				for (BYTE greenIndex = 1; greenIndex <= MAXSIDEINDEX; ++greenIndex) {
					volatile UINT line = 0;
					volatile UINT lineAlpha = 0;
					volatile UINT lineRed = 0;
					volatile UINT lineGreen = 0;
					volatile UINT lineBlue = 0;
					volatile float line2 = 0.0f;

					for (BYTE blueIndex = 1; blueIndex <= MAXSIDEINDEX; ++blueIndex) {
						const UINT index = Index(alphaIndex, redIndex, greenIndex, blueIndex);
						line += data.weights[index];
						lineAlpha += data.momentsAlpha[index];
						lineRed += data.momentsRed[index];
						lineGreen += data.momentsGreen[index];
						lineBlue += data.momentsBlue[index];
						line2 += data.moments[index];

						area[blueIndex] += line;
						areaAlpha[blueIndex] += lineAlpha;
						areaRed[blueIndex] += lineRed;
						areaGreen[blueIndex] += lineGreen;
						areaBlue[blueIndex] += lineBlue;
						area2[blueIndex] += line2;

						const UINT rgbIndex = Index(redIndex, greenIndex, blueIndex);
						const UINT prevRgbIndex = Index(redIndex - 1, greenIndex, blueIndex);
						xarea[rgbIndex] = xarea[prevRgbIndex] + area[blueIndex];
						xareaAlpha[rgbIndex] = xareaAlpha[prevRgbIndex] + areaAlpha[blueIndex];
						xareaRed[rgbIndex] = xareaRed[prevRgbIndex] + areaRed[blueIndex];
						xareaGreen[rgbIndex] = xareaGreen[prevRgbIndex] + areaGreen[blueIndex];
						xareaBlue[rgbIndex] = xareaBlue[prevRgbIndex] + areaBlue[blueIndex];
						xarea2[rgbIndex] = xarea2[prevRgbIndex] + area2[blueIndex];

						const UINT prevIndex = Index(alphaIndex - 1, redIndex, greenIndex, blueIndex);
						data.weights[index] = data.weights[prevIndex] + xarea[rgbIndex];
						data.momentsAlpha[index] = data.momentsAlpha[prevIndex] + xareaAlpha[rgbIndex];
						data.momentsRed[index] = data.momentsRed[prevIndex] + xareaRed[rgbIndex];
						data.momentsGreen[index] = data.momentsGreen[prevIndex] + xareaGreen[rgbIndex];
						data.momentsBlue[index] = data.momentsBlue[prevIndex] + xareaBlue[rgbIndex];
						data.moments[index] = data.moments[prevIndex] + xarea2[rgbIndex];
					}
				}
			}
		}
	}

	CubeCut Maximize(const ColorData& data, const Box& cube, Pixel direction, BYTE first, BYTE last, UINT wholeAlpha, UINT wholeRed, UINT wholeGreen, UINT wholeBlue, UINT wholeWeight)
	{
		auto bottomAlpha = Bottom(cube, direction, data.momentsAlpha.get());
		auto bottomRed = Bottom(cube, direction, data.momentsRed.get());
		auto bottomGreen = Bottom(cube, direction, data.momentsGreen.get());
		auto bottomBlue = Bottom(cube, direction, data.momentsBlue.get());
		auto bottomWeight = Bottom(cube, direction, data.weights.get());

		volatile bool valid = false;
		volatile auto result = 0.0f;
		volatile BYTE cutPoint = 0;

#pragma omp parallel for
		for (int position = first; position < last; ++position)
		{
			auto halfAlpha = bottomAlpha + Top(cube, direction, position, data.momentsAlpha.get());
			auto halfRed = bottomRed + Top(cube, direction, position, data.momentsRed.get());
			auto halfGreen = bottomGreen + Top(cube, direction, position, data.momentsGreen.get());
			auto halfBlue = bottomBlue + Top(cube, direction, position, data.momentsBlue.get());
			auto halfWeight = bottomWeight + Top(cube, direction, position, data.weights.get());

			if (halfWeight == 0)
				continue;

			auto halfDistance = sqr(halfAlpha) + sqr(halfRed) + sqr(halfGreen) + sqr(halfBlue);
			auto temp = halfDistance / halfWeight;

			halfAlpha = wholeAlpha - halfAlpha;
			halfRed = wholeRed - halfRed;
			halfGreen = wholeGreen - halfGreen;
			halfBlue = wholeBlue - halfBlue;
			halfWeight = wholeWeight - halfWeight;

			if (halfWeight != 0) {
				halfDistance = sqr(halfAlpha) + sqr(halfRed) + sqr(halfGreen) + sqr(halfBlue);
				temp += halfDistance / halfWeight;

				if (temp > result) {
					valid = true;
					result = temp;
					cutPoint = position;
				}
			}
		}

		return CubeCut(valid, cutPoint, result);
	}

	bool Cut(const ColorData& data, Box& first, Box& second)
	{
		auto wholeAlpha = Volume(first, data.momentsAlpha.get());
		auto wholeRed = Volume(first, data.momentsRed.get());
		auto wholeGreen = Volume(first, data.momentsGreen.get());
		auto wholeBlue = Volume(first, data.momentsBlue.get());
		auto wholeWeight = Volume(first, data.weights.get());

		auto maxAlpha = Maximize(data, first, Alpha, static_cast<BYTE>(first.AlphaMinimum + 1), first.AlphaMaximum, wholeAlpha, wholeRed, wholeGreen, wholeBlue, wholeWeight);
		auto maxRed = Maximize(data, first, Red, static_cast<BYTE>(first.RedMinimum + 1), first.RedMaximum, wholeAlpha, wholeRed, wholeGreen, wholeBlue, wholeWeight);
		auto maxGreen = Maximize(data, first, Green, static_cast<BYTE>(first.GreenMinimum + 1), first.GreenMaximum, wholeAlpha, wholeRed, wholeGreen, wholeBlue, wholeWeight);
		auto maxBlue = Maximize(data, first, Blue, static_cast<BYTE>(first.BlueMinimum + 1), first.BlueMaximum, wholeAlpha, wholeRed, wholeGreen, wholeBlue, wholeWeight);

		Pixel direction = Blue;
		if ((maxAlpha.value >= maxRed.value) && (maxAlpha.value >= maxGreen.value) && (maxAlpha.value >= maxBlue.value)) {
			if (!maxAlpha.valid)
				return false;
			direction = Alpha;
		}
		else if ((maxRed.value >= maxAlpha.value) && (maxRed.value >= maxGreen.value) && (maxRed.value >= maxBlue.value))
			direction = Red;
		else if ((maxGreen.value >= maxAlpha.value) && (maxGreen.value >= maxRed.value) && (maxGreen.value >= maxBlue.value))
			direction = Green;

		second.AlphaMaximum = first.AlphaMaximum;
		second.RedMaximum = first.RedMaximum;
		second.GreenMaximum = first.GreenMaximum;
		second.BlueMaximum = first.BlueMaximum;

		switch (direction)
		{
		case Alpha:
			second.AlphaMinimum = first.AlphaMaximum = maxAlpha.position;
			second.RedMinimum = first.RedMinimum;
			second.GreenMinimum = first.GreenMinimum;
			second.BlueMinimum = first.BlueMinimum;
			break;

		case Red:
			second.RedMinimum = first.RedMaximum = maxRed.position;
			second.AlphaMinimum = first.AlphaMinimum;
			second.GreenMinimum = first.GreenMinimum;
			second.BlueMinimum = first.BlueMinimum;
			break;

		case Green:
			second.GreenMinimum = first.GreenMaximum = maxGreen.position;
			second.AlphaMinimum = first.AlphaMinimum;
			second.RedMinimum = first.RedMinimum;
			second.BlueMinimum = first.BlueMinimum;
			break;

		case Blue:
			second.BlueMinimum = first.BlueMaximum = maxBlue.position;
			second.AlphaMinimum = first.AlphaMinimum;
			second.RedMinimum = first.RedMinimum;
			second.GreenMinimum = first.GreenMinimum;
			break;
		}

		first.Size = (first.AlphaMaximum - first.AlphaMinimum) * (first.RedMaximum - first.RedMinimum) * (first.GreenMaximum - first.GreenMinimum) * (first.BlueMaximum - first.BlueMinimum);
		second.Size = (second.AlphaMaximum - second.AlphaMinimum) * (second.RedMaximum - second.RedMinimum) * (second.GreenMaximum - second.GreenMinimum) * (second.BlueMaximum - second.BlueMinimum);

		return true;
	}

	float CalculateVariance(const ColorData& data, const Box& cube)
	{
		auto volumeAlpha = Volume(cube, data.momentsAlpha.get());
		auto volumeRed = Volume(cube, data.momentsRed.get());
		auto volumeGreen = Volume(cube, data.momentsGreen.get());
		auto volumeBlue = Volume(cube, data.momentsBlue.get());
		auto volumeMoment = Volume(cube, data.moments.get());
		auto volumeWeight = Volume(cube, data.weights.get());

		float distance = sqr(volumeAlpha) + sqr(volumeRed) + sqr(volumeGreen) + sqr(volumeBlue);

		return volumeWeight != 0.0f ? (volumeMoment - distance / volumeWeight) : 0.0f;
	}

	void SplitData(vector<Box>& boxList, UINT& colorCount, ColorData& data)
	{
		int next = 0;
		auto volumeVariance = make_unique<float[]>(colorCount);
		boxList.resize(colorCount);
		boxList[0].AlphaMaximum = MAXSIDEINDEX;
		boxList[0].RedMaximum = MAXSIDEINDEX;
		boxList[0].GreenMaximum = MAXSIDEINDEX;
		boxList[0].BlueMaximum = MAXSIDEINDEX;

		for (int cubeIndex = 1; cubeIndex < colorCount; ++cubeIndex) {
			if (Cut(data, boxList[next], boxList[cubeIndex])) {
				volumeVariance[next] = boxList[next].Size > 1 ? CalculateVariance(data, boxList[next]) : 0.0f;
				volumeVariance[cubeIndex] = boxList[cubeIndex].Size > 1 ? CalculateVariance(data, boxList[cubeIndex]) : 0.0f;
			}
			else {
				volumeVariance[next] = 0.0f;
				cubeIndex--;
			}

			next = 0;
			auto temp = volumeVariance[0];

			for (int index = 1; index <= cubeIndex; ++index) {
				if (volumeVariance[index] <= temp)
					continue;
				temp = volumeVariance[index];
				next = index;
			}

			if (temp > 0.0f)
				continue;

			colorCount = cubeIndex + 1;
			break;
		}
		boxList.resize(colorCount);
	}

	void BuildLookups(ColorPalette* pPalette, vector<Box>& cubes, const ColorData& data)
	{
		volatile UINT lookupsCount = 0;
		if (m_transparentPixelIndex >= 0)
			pPalette->Entries[lookupsCount++] = m_transparentColor;
			
		for (auto const& cube : cubes) {
			auto weight = Volume(cube, data.weights.get());

			if (weight <= 0)
				continue;

			BYTE alpha = static_cast<BYTE>(Volume(cube, data.momentsAlpha.get()) / weight);
			BYTE red = static_cast<BYTE>(Volume(cube, data.momentsRed.get()) / weight);
			BYTE green = static_cast<BYTE>(Volume(cube, data.momentsGreen.get()) / weight);
			BYTE blue = static_cast<BYTE>(Volume(cube, data.momentsBlue.get()) / weight);
			pPalette->Entries[lookupsCount++] = Color::MakeARGB(alpha, red, green, blue);
		}

		if(lookupsCount < pPalette->Count)
			pPalette->Count = lookupsCount;
	}

	unsigned short closestColorIndex(const ColorPalette* pPalette, const UINT nMaxColors, const ARGB argb)
	{
		UINT k = 0;
		Color c(argb);
		vector<unsigned short> closest(5);
		auto got = closestMap.find(argb);
		if (got == closestMap.end()) {
			closest[2] = closest[3] = SHORT_MAX;

			for (; k < nMaxColors; k++) {
				Color c2(pPalette->Entries[k]);
				closest[4] = abs(c.GetA() - c2.GetA()) + abs(c.GetR() - c2.GetR()) + abs(c.GetG() - c2.GetG()) + abs(c.GetB() - c2.GetB());
				if (closest[4] < closest[2]) {
					closest[1] = closest[0];
					closest[3] = closest[2];
					closest[0] = k;
					closest[2] = closest[4];
				}
				else if (closest[4] < closest[3]) {
					closest[1] = k;
					closest[3] = closest[4];
				}
			}

			if (closest[3] == SHORT_MAX)
				closest[2] = 0;
		}
		else
			closest = got->second;

		if (closest[2] == 0 || (rand() % (closest[3] + closest[2])) <= closest[3])
			k = closest[0];
		else
			k = closest[1];

		closestMap[argb] = closest;
		return k;
	}

	unsigned short nearestColorIndex(const ColorPalette* pPalette, const ARGB argb, const BYTE alphaThreshold)
	{
		Color c(argb);
		unsigned short k = 0;
		if (c.GetA() <= alphaThreshold)
			return k;

		auto got = nearestMap.find(argb);
		if (got == nearestMap.end()) {
			double mindist = SHORT_MAX;
			for (UINT i = 0; i < pPalette->Count; i++) {
				Color c2(pPalette->Entries[i]);
				double curdist = sqr(c2.GetA() - c.GetA());
				if (curdist > mindist)
					continue;

				curdist += PR * sqr(c2.GetR() - c.GetR());
				if (curdist > mindist)
					continue;

				curdist += PG * sqr(c2.GetG() - c.GetG());
				if (curdist > mindist)
					continue;

				curdist += PB * sqr(c2.GetB() - c.GetB());
				if (curdist > mindist)
					continue;

				mindist = curdist;
				k = i;
			}

			nearestMap[argb] = k;
		}
		else
			k = got->second;

		return k;
	}

	void GetQuantizedPalette(const ColorData& data, ColorPalette* pPalette, const UINT colorCount, const BYTE alphaThreshold)
	{
		auto alphas = make_unique<UINT[]>(colorCount);
		auto reds = make_unique<UINT[]>(colorCount);
		auto greens = make_unique<UINT[]>(colorCount);
		auto blues = make_unique<UINT[]>(colorCount);
		auto sums = make_unique<UINT[]>(colorCount);

		int pixelsCount = data.pixelsCount;

		for (UINT pixelIndex = 0; pixelIndex < pixelsCount; ++pixelIndex) {
			auto argb = data.pixels[pixelIndex];
			Color pixel(argb);
			if (pixel.GetA() <= alphaThreshold)
				continue;

			UINT bestMatch = nearestColorIndex(pPalette, argb, alphaThreshold);

			alphas[bestMatch] += pixel.GetA();
			reds[bestMatch] += pixel.GetR();
			greens[bestMatch] += pixel.GetG();
			blues[bestMatch] += pixel.GetB();
			sums[bestMatch]++;
		}
		nearestMap.clear();

		short paletteIndex = (m_transparentPixelIndex < 0) ? 0 : 1;
		for (; paletteIndex < colorCount; ++paletteIndex) {
			if (sums[paletteIndex] > 0) {
				alphas[paletteIndex] /= sums[paletteIndex];
				reds[paletteIndex] /= sums[paletteIndex];
				greens[paletteIndex] /= sums[paletteIndex];
				blues[paletteIndex] /= sums[paletteIndex];
			}

			pPalette->Entries[paletteIndex] = Color::MakeARGB(alphas[paletteIndex], reds[paletteIndex], greens[paletteIndex], blues[paletteIndex]);
		}
	}

	bool quantize_image(const ARGB* pixels, const ColorPalette* pPalette, unsigned short* qPixels, const UINT width, const UINT height, const bool dither, BYTE alphaThreshold)
	{
		if (dither) {
			UINT pixelIndex = 0;

			const int DJ = 4;
			const int BLOCK_SIZE = 256;
			const int DITHER_MAX = 20;
			const int err_len = (width + 2) * DJ;
			auto clamp = make_unique <BYTE[]>(DJ * BLOCK_SIZE);
            auto erowErr = make_unique<int[]>(err_len);
            auto orowErr = make_unique<int[]>(err_len);
			auto limtb = make_unique<char[]>(2 * BLOCK_SIZE);
			auto pDitherPixel = make_unique<int[]>(DJ);

			for (int i = 0; i < BLOCK_SIZE; ++i) {
				clamp[i] = 0;
				clamp[i + BLOCK_SIZE] = static_cast<BYTE>(i);
				clamp[i + BLOCK_SIZE * 2] = BYTE_MAX;
				clamp[i + BLOCK_SIZE * 3] = BYTE_MAX;

				limtb[i] = -DITHER_MAX;
				limtb[i + BLOCK_SIZE] = DITHER_MAX;
			}
			for (int i = -DITHER_MAX; i <= DITHER_MAX; i++)
				limtb[i + BLOCK_SIZE] = i;

			auto row0 = erowErr.get();
			auto row1 = orowErr.get();

			bool noBias = hasSemiTransparency || pPalette->Count < 64;
			int dir = 1;
			for (int i = 0; i < height; ++i) {
				if (dir < 0)
					pixelIndex += width - 1;

				int cursor0 = DJ, cursor1 = width * DJ;
				row1[cursor1] = row1[cursor1 + 1] = row1[cursor1 + 2] = row1[cursor1 + 3] = 0;
				for (UINT j = 0; j < width; ++j) {
					Color c(pixels[pixelIndex]);

					CalcDitherPixel(pDitherPixel.get(), c, clamp.get(), row0, cursor0, noBias);
					int r_pix = pDitherPixel[0];
					int g_pix = pDitherPixel[1];
					int b_pix = pDitherPixel[2];
					int a_pix = pDitherPixel[3];
					auto argb = Color::MakeARGB(a_pix, r_pix, g_pix, b_pix);
					Color c1(argb);
					qPixels[pixelIndex] = nearestColorIndex(pPalette, argb, alphaThreshold);

					Color c2(pPalette->Entries[qPixels[pixelIndex]]);

					r_pix = limtb[c1.GetR() - c2.GetR() + BLOCK_SIZE];
					g_pix = limtb[c1.GetG() - c2.GetG() + BLOCK_SIZE];
					b_pix = limtb[c1.GetB() - c2.GetB() + BLOCK_SIZE];
					a_pix = limtb[c1.GetA() - c2.GetA() + BLOCK_SIZE];

					int k = r_pix * 2;
					row1[cursor1 - DJ] = r_pix;
					row1[cursor1 + DJ] += (r_pix += k);
					row1[cursor1] += (r_pix += k);
					row0[cursor0 + DJ] += (r_pix += k);

					k = g_pix * 2;
					row1[cursor1 + 1 - DJ] = g_pix;
					row1[cursor1 + 1 + DJ] += (g_pix += k);
					row1[cursor1 + 1] += (g_pix += k);
					row0[cursor0 + 1 + DJ] += (g_pix += k);

					k = b_pix * 2;
					row1[cursor1 + 2 - DJ] = b_pix;
					row1[cursor1 + 2 + DJ] += (b_pix += k);
					row1[cursor1 + 2] += (b_pix += k);
					row0[cursor0 + 2 + DJ] += (b_pix += k);

					k = a_pix * 2;
					row1[cursor1 + 3 - DJ] = a_pix;
					row1[cursor1 + 3 + DJ] += (a_pix += k);
					row1[cursor1 + 3] += (a_pix += k);
					row0[cursor0 + 3 + DJ] += (a_pix += k);

					cursor0 += DJ;
					cursor1 -= DJ;
					pixelIndex += dir;
				}
				if ((i % 2) == 1)
					pixelIndex += width + 1;

				dir *= -1;
				swap(row0, row1);
			}
			return true;
		}

		for (int i = 0; i < (width * height); ++i)
			qPixels[i] = closestColorIndex(pPalette, pPalette->Count, pixels[i]);

		return true;
	}
	
	bool WuQuantizer::QuantizeImage(Bitmap* pSource, Bitmap* pDest, UINT& nMaxColors, bool dither, BYTE alphaThreshold, BYTE alphaFader)
	{
		const UINT bitmapWidth = pSource->GetWidth();
		const UINT bitmapHeight = pSource->GetHeight();

		auto pPaletteBytes = make_unique<BYTE[]>(sizeof(ColorPalette) + nMaxColors * sizeof(ARGB));
		auto pPalette = (ColorPalette*)pPaletteBytes.get();
		pPalette->Count = nMaxColors;
		
		if (nMaxColors <= 32)
			PR = PG = PB = 1;

		auto qPixels = make_unique<unsigned short[]>(bitmapWidth * bitmapHeight);
		if (nMaxColors > 2) {
			ColorData colorData(SIDESIZE, bitmapWidth, bitmapHeight);
			BuildHistogram(colorData, pSource, alphaThreshold, alphaFader);
			CalculateMoments(colorData);
			vector<Box> cubes;
			SplitData(cubes, nMaxColors, colorData);

			BuildLookups(pPalette, cubes, colorData);
			cubes.clear();

			nMaxColors = pPalette->Count;
			if (nMaxColors > 16 && nMaxColors <= 256 && pDest->GetPixelFormat() != PixelFormat8bppIndexed)
				pDest->ConvertFormat(PixelFormat8bppIndexed, DitherTypeSolid, PaletteTypeCustom, pPalette, 0);

			GetQuantizedPalette(colorData, pPalette, nMaxColors, alphaThreshold);
			if (nMaxColors > 256) {
				auto qPixels = make_unique<ARGB[]>(bitmapWidth * bitmapHeight);
				dithering_image(colorData.GetPixels(), pPalette, closestColorIndex, hasSemiTransparency, m_transparentPixelIndex, nMaxColors, qPixels.get(), bitmapWidth, bitmapHeight);
				return ProcessImagePixels(pDest, qPixels.get(), hasSemiTransparency, m_transparentPixelIndex);
			}			
			quantize_image(colorData.GetPixels(), pPalette, qPixels.get(), bitmapWidth, bitmapHeight, dither, alphaThreshold);
		}
		else {
			vector<ARGB> pixels(bitmapWidth * bitmapHeight);
			GrabPixels(pSource, pixels, hasSemiTransparency, m_transparentPixelIndex, m_transparentColor);
			if (m_transparentPixelIndex >= 0) {
				pPalette->Entries[0] = m_transparentColor;
				pPalette->Entries[1] = Color::Black;
			}
			else {
				pPalette->Entries[0] = Color::Black;
				pPalette->Entries[1] = Color::White;
			}
			quantize_image(pixels.data(), pPalette, qPixels.get(), bitmapWidth, bitmapHeight, dither, alphaThreshold);
		}		
		
		if (m_transparentPixelIndex >= 0) {
			UINT k = qPixels[m_transparentPixelIndex];
			if (nMaxColors > 2)
				pPalette->Entries[k] = m_transparentColor;
			else if (pPalette->Entries[k] != m_transparentColor)
				swap(pPalette->Entries[0], pPalette->Entries[1]);
		}
		closestMap.clear();
		nearestMap.clear();

		return ProcessImagePixels(pDest, pPalette, qPixels.get(), m_transparentPixelIndex >= 0);
	}

}

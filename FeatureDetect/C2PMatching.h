#pragma once

#include "opencv.hpp"
#include "ValidPoint.h"

using namespace cv;

inline std::vector<KeyPoint> Vec2d2KeyPoint(const std::vector<Vec2d> input, const float scale = 5) {
	std::vector<KeyPoint> kpVec;
	for (auto p = input.begin(); p != input.end(); ++p) {
		kpVec.push_back(KeyPoint((float)((*p)[0]) * scale, (float)((*p)[1]) * scale, 1));
	}
	return kpVec;
}

inline std::vector<KeyPoint> ValidPoint2KeyPoint(const std::vector<ValidPoint> input, const int scale = 1) {
	std::vector<KeyPoint> kpVec;
	for (auto p = input.begin(); p != input.end(); ++p) {
		kpVec.push_back(KeyPoint((float)(p->position[0]) * scale, (float)(p->position[1]) * scale, 1));
	}
	return kpVec;
}

inline std::vector<DMatch> Vec2i2Dmatch(const std::vector<Vec2i> input) {
	std::vector<DMatch> dmVec;
	for (auto p = input.begin(); p != input.end(); ++p) {
		dmVec.push_back(DMatch((float)((*p)[0]) , (float)((*p)[1]) , 1));
	}
	return dmVec;
}

void C2PMatching(const std::vector<ValidPoint> vpType1, const std::vector<ValidPoint> vpType2, const Mat &camImg);
#pragma once

#include "opencv.hpp";

struct ValidPoint
{
	int type;
	cv::Vec2d position;
	int idxC = -1;
	int idxTL = -1;
	int idxBL = -1;
	int idxTR = -1;
	int idxBR = -1;
	ValidPoint* C_ptr = nullptr;
	ValidPoint* TL_ptr = nullptr;
	ValidPoint* BL_ptr = nullptr;
	ValidPoint* TR_ptr = nullptr;
	ValidPoint* BR_ptr = nullptr;
	std::vector<ValidPoint*> T_ptr_list;
	std::vector<ValidPoint*> B_ptr_list;
	std::vector<ValidPoint*> L_ptr_list;
	std::vector<ValidPoint*> R_ptr_list;
	int eleLeft = -1;
	int eleRight = -1;
	int eleTop = -1;
	int eleBottom = -1;
	std::vector<cv::Vec4i> codewords;

};
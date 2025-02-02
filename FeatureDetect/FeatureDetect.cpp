// FeatureDetect.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "ColorNaming.h"
#include "cmdLine.h"
#include "ValidPoint.h"
#include "C2PMatching.h"
#include "global.h";

using namespace cv;


int MASK_SIZE = 18;
float scale = 1.0;

Mat in_img;
Mat temp_img;
Mat1b gray;

Mat1f dValueMatGray;
std::vector<Vec2d> detectedGridPoint;
std::vector<Vec2d> detectedGridPointType1;
std::vector<Vec2d> detectedGridPointType2;
std::vector<int> detectedGridPointType;
std::vector<ValidPoint> validPointsType1;
std::vector<ValidPoint> validPointsType2;

std::string imgFileName;//global
std::string outFilePrefix; //global

void Th_d_onChange(int value, void* userdata) {
	std::vector<Mat> mat_data = *(std::vector<Mat>*)(userdata);
	Mat3b img;// = mat_data[0].clone();
	mat_data[0].copyTo(img);
	Mat1f dValueMat = (Mat1f)mat_data[1];
	Mat1f out;
	threshold(dValueMat, out, value, 1, THRESH_BINARY);
	for (int i = 0; i < out.cols; i++) {
		for (int j = 0; j < out.rows; j++) {
			if (out(j, i) != 0) {
				circle(img, Point(i, j), 1, Scalar(0, 0, 255), -1);
			}
		}
	}
	imshow("Candidate Grid Points", img);
}
void CandidateGP(const Mat3b& input, Mat& output) {

	int L = MASK_SIZE; //mask size;
	std::vector<Mat> bgr;
	split(input, bgr);
	imshow("B", bgr[0]);
	imshow("G", bgr[1]);
	imshow("R", bgr[2]);
	
	Mat1f B_f, G_f, R_f;
	Mat temp;
	bgr[0].convertTo(B_f, CV_32FC1);
	bgr[1].convertTo(G_f, CV_32FC1);
	bgr[2].convertTo(R_f, CV_32FC1);
	
	Mat1b Gray;
	Mat1f Gray_f;
	//Gray = bgr[0] / 3.f + bgr[1] / 3.f + bgr[2] / 3.f;
 	cvtColor(input, Gray, COLOR_BGR2GRAY);
	GaussianBlur(Gray, Gray, Size(3, 3), 0);
	//threshold(Gray, Gray, 200, 255,  ThresholdTypes::THRESH_BINARY);
	imshow("gray binary", Gray);
	Gray.convertTo(Gray_f, CV_32FC1);

	Mat1f dValueMat, dValueMat_bin;
	Mat1f B_out, G_out, R_out, Gray_out;

	Mat1f K = Mat1f::zeros(Size(2 * L + 1, 2 * L + 1));//convolve mask
	K.row(L).setTo(1.f);									//. -1  . 
	K.col(L).setTo(-1.f);									//1  0  1
	K.at<float>(L, L) = 0;								//. -1  .

	//K.row(L-1).setTo(1);
	//K.col(L-1).setTo(-1);
	//K.row(L+1).setTo(1);
	//K.col(L+1).setTo(-1);
	//K(Rect(L - 1, L - 1, 3, 3)).setTo(0);
	std::cout << "K = \n" << K << std::endl;

	filter2D(B_f, B_out, CV_32FC1, K, Point(-1, -1), 0, BORDER_REPLICATE);
	filter2D(G_f, G_out, CV_32FC1, K, Point(-1, -1), 0, BORDER_REPLICATE);
	filter2D(R_f, R_out, CV_32FC1, K, Point(-1, -1), 0, BORDER_REPLICATE);
	filter2D(Gray_f, Gray_out, CV_32FC1, K, Point(-1, -1), 0, BORDER_REPLICATE);
	dValueMatGray = Gray_out;
	B_out = abs(B_out);
	G_out = abs(G_out);
	R_out = abs(R_out);

	dValueMat = max(max(B_out, G_out), R_out) / (2 * L);
	//dValueMat = abs(Gray_out);

	Mat1f K_v = Mat1f::ones(Size(1, 2 * L + 1));//convolve mask
	Mat1f K_h = Mat1f::ones(Size(2 * L + 1, 1));//convolve mask
	K_v(Rect(0, 0, 1, 2 * L)).setTo(-1);
	K_h(Rect(0, 0, 2 * L, 1)).setTo(-1);
	K_v.at<float>(L, 0) = 0;
	K_h.at<float>(0, L) = 0;
	

	Mat1b borderMask = Mat1b::ones(dValueMat.size()); //Mask out the results on the border pixels
	borderMask(Rect(L, L, dValueMat.cols - 2 * L, dValueMat.rows - 2 * L)).setTo(0);
	dValueMat.setTo(0, borderMask);

	namedWindow("Candidate Grid Points");
	imshow("Candidate Grid Points", input);
	int th_d = 50;// 1000;
	int th_d_max = 255;//255 *(2 * L + 1) * 3;
	std::vector<Mat> userdata;
	userdata.push_back(input);
	userdata.push_back(dValueMat);
	createTrackbar("th_d", "Candidate Grid Points", &th_d, th_d_max, Th_d_onChange, &userdata);
	Th_d_onChange(th_d, &userdata);

	waitKey();
	threshold(dValueMat, dValueMat_bin, th_d, 1, THRESH_BINARY);
	
	Mat dValueMat_byte;
	dValueMat_bin.convertTo(dValueMat_byte, CV_8UC1);
	findNonZero(dValueMat_byte, output);

}

Mat CircRoi(Mat& img, Rect& region) {
	Mat rectRoi = img(region);
	Mat circMask(rectRoi.size(), CV_8UC1, Scalar(0));
	Point center = Point((circMask.cols - 1) / 2, (circMask.rows - 1) / 2);
	int r = (min(circMask.cols, circMask.rows) - 1) / 2;
	circle(circMask, center, r, Scalar(255), -1, LINE_AA); // filled circle
	Mat circRoi;
	rectRoi.copyTo(circRoi, circMask);
	return circRoi;
	//bitwise_and(roi, roi, circRoi, mask); // retain only pixels inside the circle
}

double PMCC(Mat1b& roi, int n, bool thresholding = false) {
	double a_sum = 0;
	double b_sum = 0;
	double ab_sum = 0;
	double a2_sum = 0;
	double b2_sum = 0;

	Mat _roi;
	if (thresholding)
		threshold(roi, _roi, 0, 255, ThresholdTypes::THRESH_OTSU);
		//equalizeHist(roi, _roi);
	else
		_roi = roi;
	
	for (int i = 0; i < _roi.cols; i++) {
		for (int j = 0; j < _roi.rows; j++) {
			double a = _roi.at<uchar>(j, i);
			double b = _roi.at<uchar>(_roi.rows - j - 1, _roi.cols - i - 1);
			a_sum += a;
			b_sum += b;
			ab_sum += a * b;
			a2_sum += a * a;
			b2_sum += b * b;
		}
	}

	double rho = n * ab_sum - a_sum * b_sum;
	rho /= sqrt(n*a2_sum - a_sum * a_sum) * sqrt(n*b2_sum - b_sum * b_sum);
	/*imshow("otsu", _roi);
	std::cout << rho << std::endl;
	waitKey();*/
	return rho;
}

//MSD not working poor coefficient
double MSD(Mat1b& roi, int n, const Mat& mask, bool thresholding = true) {
	double a_minus_b_2_sum = 0;
	double a_minus_avg_2_sum = 0;
	Mat _roi;
	if (thresholding)
		threshold(roi, _roi, 0, 255, ThresholdTypes::THRESH_OTSU);
	else
		_roi = roi;
	double avg = sum(_roi)[0] / (double)n;
	for (int i = 0; i < _roi.cols; i++) {
		for (int j = 0; j < _roi.rows; j++) {
			//if (mask.at<uchar>(j, i) == 0) continue;

			double a = _roi.at<uchar>(j, i);
			double b = _roi.at<uchar>(_roi.rows - j - 1, _roi.cols - i - 1);
			a_minus_b_2_sum += (a - b)*(a - b);
			a_minus_avg_2_sum += (a - avg)*(a - avg);
		}
	}

	double rho = a_minus_b_2_sum / a_minus_avg_2_sum;

	return 1 - rho;
}

void RefineGP(const Mat1b& grayImg, Mat& gpCandidates, String ccMethod) {
	float roi_r = MASK_SIZE;
	Point center(roi_r, roi_r);
	Mat circMask(roi_r * 2 + 1, roi_r * 2 + 1, CV_8UC1, Scalar(0));
	circle(circMask, center, roi_r, Scalar(255), -1, LINE_8); // filled circle
	int n = countNonZero(circMask);
	//imshow("circMask", circMask);

	Mat1f rhoValueMat(grayImg.size(), 0);
	if (ccMethod == "PMCC") {
		for (auto i = gpCandidates.begin<Point>(); i != gpCandidates.end<Point>(); ++i) {
			int rect_x = (*i).x - roi_r;
			int rect_y = (*i).y - roi_r;
			Mat1b rectRoi = grayImg(Rect(rect_x, rect_y, roi_r * 2 + 1, roi_r * 2 + 1));
			Mat1b circRoi;
			rectRoi.copyTo(circRoi, circMask);
			//normalize(circRoi,circRoi,)
			double rho = PMCC(circRoi, n);
			//double rho = PMCC(rectRoi, (roi_r * 2 + 1)*(roi_r * 2 + 1));

			//double rho = MSD(circRoi, n);
			rhoValueMat.at<float>(*i) = rho;
			//imshow("circRoi", circRoi);
			//waitKey();
		}
	}
	else if (ccMethod == "MSD") {
		for (auto i = gpCandidates.begin<Point>(); i != gpCandidates.end<Point>(); ++i) {
			int rect_x = (*i).x - roi_r;
			int rect_y = (*i).y - roi_r;
			Mat1b rectRoi = grayImg(Rect(rect_x, rect_y, roi_r * 2 + 1, roi_r * 2 + 1));
			Mat1b circRoi;
			//rectRoi.copyTo(circRoi, circMask);
			rectRoi.copyTo(circRoi);
			n = (roi_r * 2 + 1) * (roi_r * 2 + 1);
			double rho = MSD(circRoi, n, circMask);
			rhoValueMat.at<float>(*i) = rho;
			//std::cout << rho << ", ";
		}
	}
	//normalize(rhoValueMat, rhoValueMat, 1, 0, NORM_MINMAX);
	Mat1f rhoValueMat_bw;
	Mat1i rhoValueMat_lables;

	//find clusters
	threshold(rhoValueMat, rhoValueMat_bw, 0, 1, THRESH_BINARY);
	Mat1b rhoValueMat_bw_8bit;
	rhoValueMat_bw.convertTo(rhoValueMat_bw_8bit, CV_8UC1, 255, 0);
	int LableCnt = connectedComponents(rhoValueMat_bw_8bit, rhoValueMat_lables);
	std::cout << "LableCnt = " << LableCnt << std::endl;
	//scan the lable image and find the loacal maximum for each cluster
	Mat1f rhoValueMat_maxValue(1, LableCnt, 0.f);
	Mat2i rhoValueMat_maxPosition(1, LableCnt);
	for (int i = 0; i < rhoValueMat_lables.rows; ++i)
	{
		for (int j = 0; j < rhoValueMat_lables.cols; ++j)
		{
			int _lable = rhoValueMat_lables(i, j);
			float _value = rhoValueMat(i, j);
			if (_value > rhoValueMat_maxValue(_lable)) {
				rhoValueMat_maxValue(_lable) = _value;
				rhoValueMat_maxPosition(_lable) = Vec2i(j, i);
			}
		}
	}
	//refine each local maximum to sub pixel
	int windowSize = 3;
	float rho_th = 0.5;
	std::vector<Vec2d> refined;
	std::vector<int> refined_type;//0,1
	for (int k = 1; k < rhoValueMat_maxPosition.cols; ++k) {
		if (rhoValueMat_maxValue(k) < rho_th)
			continue;

		Vec2i _p = rhoValueMat_maxPosition(k);
		Vec2i _tl = _p - Vec2i((windowSize - 1) / 2, (windowSize - 1) / 2);//top left point
		Mat1f roi = rhoValueMat(Rect(_tl[0], _tl[1], windowSize, windowSize));
		float rho_sum = sum(roi)[0];
		Vec2d coordinate_rho_sum = Vec2d(0, 0);
		for (int i = 0; i < windowSize; ++i) {
			for (int j = 0; j < windowSize; ++j) {
				coordinate_rho_sum += Vec2d(_tl[0] + i, _tl[1] + j)*roi(j, i);
			}
		}
		Vec2d  sub_pixel = coordinate_rho_sum / rho_sum;
		int type;
		if (dValueMatGray(_p[1], _p[0]) < 0) {
			type = 0; //p1
			detectedGridPointType1.push_back(sub_pixel);
		}
		else {
			type = 1; //p2
			detectedGridPointType2.push_back(sub_pixel);
		}

		refined.push_back(sub_pixel);
		refined_type.push_back(type);
		//std::cout << "candidate : " << _p << std::endl;
		//std::cout << "refined : " << sub_pixel << std::endl;
		//std::cout << std::endl;

	}
	std::cout << "grid points detected : " << refined.size() << std::endl;



	Mat1b rhoValueMat_gray;
	rhoValueMat.convertTo(rhoValueMat_gray, CV_8UC1, 255);
	normalize(rhoValueMat_gray, rhoValueMat_gray, 0, 255, NORM_MINMAX);
	Mat rhoValueMat_color;
	applyColorMap(rhoValueMat_gray, rhoValueMat_color, COLORMAP_JET);
	imshow("rhoValueMat_color", rhoValueMat_color);
	Mat detectedGridPointImg;
	in_img.copyTo(detectedGridPointImg);
	for (auto _p = refined.begin(); _p != refined.end(); ++_p) {
		int idx = std::distance(refined.begin(), _p);
		Scalar color;
		if (refined_type[idx] == 0)
			color = Scalar(0, 0, 255);
		else
			color = Scalar(255, 0, 0);

		circle(detectedGridPointImg, Point((int)round((*_p)[0]), (int)round((*_p)[1])), 5, color, -1);
	}

	detectedGridPoint = refined;
	detectedGridPointType = refined_type;

	imshow("detectedGridPointImg", detectedGridPointImg);

	waitKey();

}

void findTBLR(ValidPoint& p) {
	//std::vector<ValidPoint*> topPtrList, downPtrList, leftPtrList, rightPtrList;
	//std::vector<int> elements;

	//find top GP of the same type
	if (p.TL_ptr == nullptr && p.TR_ptr != nullptr) {
		if (p.TR_ptr->TL_ptr != nullptr) {
			p.T_ptr_list.push_back(p.TR_ptr->TL_ptr);
		}
	}
	else if (p.TL_ptr != nullptr && p.TR_ptr == nullptr) {
		if (p.TL_ptr->TR_ptr != nullptr) {
			p.T_ptr_list.push_back(p.TL_ptr->TR_ptr);
		}
	}
	else if (p.TL_ptr != nullptr && p.TR_ptr != nullptr) {
		if (p.TR_ptr->TL_ptr != nullptr && p.TL_ptr->TR_ptr == nullptr) {
			p.T_ptr_list.push_back(p.TR_ptr->TL_ptr);
		}
		else if (p.TR_ptr->TL_ptr == nullptr && p.TL_ptr->TR_ptr != nullptr) {
			p.T_ptr_list.push_back(p.TL_ptr->TR_ptr);
		}
		else if (p.TR_ptr->TL_ptr != nullptr && p.TL_ptr->TR_ptr != nullptr) {
			if (p.TR_ptr->TL_ptr == p.TL_ptr->TR_ptr) {
				p.T_ptr_list.push_back(p.TR_ptr->TL_ptr);
			}
			else {
				p.T_ptr_list.push_back(p.TL_ptr->TR_ptr);
				p.T_ptr_list.push_back(p.TR_ptr->TL_ptr);
			}
		}
	}

	//find bottom GP of the same type
	if (p.BL_ptr == nullptr && p.BR_ptr != nullptr) {
		if (p.BR_ptr->BL_ptr != nullptr) {
			p.B_ptr_list.push_back(p.BR_ptr->BL_ptr);
		}
	}
	else if (p.BL_ptr != nullptr && p.BR_ptr == nullptr) {
		if (p.BL_ptr->BR_ptr != nullptr) {
			p.B_ptr_list.push_back(p.BL_ptr->BR_ptr);
		}
	}
	else if (p.BL_ptr != nullptr && p.BR_ptr != nullptr) {
		if (p.BR_ptr->BL_ptr != nullptr && p.BL_ptr->BR_ptr == nullptr) {
			p.B_ptr_list.push_back(p.BR_ptr->BL_ptr);
		}
		else if (p.BR_ptr->BL_ptr == nullptr && p.BL_ptr->BR_ptr != nullptr) {
			p.B_ptr_list.push_back(p.BL_ptr->BR_ptr);
		}
		else if (p.BR_ptr->BL_ptr != nullptr && p.BL_ptr->BR_ptr != nullptr) {
			if (p.BR_ptr->BL_ptr == p.BL_ptr->BR_ptr) {
				p.B_ptr_list.push_back(p.BR_ptr->BL_ptr);
			}
			else {
				p.B_ptr_list.push_back(p.BL_ptr->BR_ptr);
				p.B_ptr_list.push_back(p.BR_ptr->BL_ptr);
			}
		}
	}

	//find left GP of the same type
	if (p.TL_ptr == nullptr && p.BL_ptr != nullptr) {
		if (p.BL_ptr->TL_ptr != nullptr) {
			p.L_ptr_list.push_back(p.BL_ptr->TL_ptr);
		}
	}
	else if (p.TL_ptr != nullptr && p.BL_ptr == nullptr) {
		if (p.TL_ptr->BL_ptr != nullptr) {
			p.L_ptr_list.push_back(p.TL_ptr->BL_ptr);
		}
	}
	else if (p.TL_ptr != nullptr && p.BL_ptr != nullptr) {
		if (p.BL_ptr->TL_ptr != nullptr && p.TL_ptr->BL_ptr == nullptr) {
			p.L_ptr_list.push_back(p.BL_ptr->TL_ptr);
		}
		else if (p.BL_ptr->TL_ptr == nullptr && p.TL_ptr->BL_ptr != nullptr) {
			p.L_ptr_list.push_back(p.TL_ptr->BL_ptr);
		}
		else if (p.BL_ptr->TL_ptr != nullptr && p.TL_ptr->BL_ptr != nullptr) {
			if (p.BL_ptr->TL_ptr == p.TL_ptr->BL_ptr) {
				p.L_ptr_list.push_back(p.BL_ptr->TL_ptr);
			}
			else {
				p.L_ptr_list.push_back(p.TL_ptr->BL_ptr);
				p.L_ptr_list.push_back(p.BL_ptr->TL_ptr);
			}
		}
	}

	//find right GP of the same type
	if (p.TR_ptr == nullptr && p.BR_ptr != nullptr) {
		if (p.BR_ptr->TR_ptr != nullptr) {
			p.R_ptr_list.push_back(p.BR_ptr->TR_ptr);
		}
	}
	else if (p.TR_ptr != nullptr && p.BR_ptr == nullptr) {
		if (p.TR_ptr->BR_ptr != nullptr) {
			p.R_ptr_list.push_back(p.TR_ptr->BR_ptr);
		}
	}
	else if (p.TR_ptr != nullptr && p.BR_ptr != nullptr) {
		if (p.BR_ptr->TR_ptr != nullptr && p.TR_ptr->BR_ptr == nullptr) {
			p.R_ptr_list.push_back(p.BR_ptr->TR_ptr);
		}
		else if (p.BR_ptr->TR_ptr == nullptr && p.TR_ptr->BR_ptr != nullptr) {
			p.R_ptr_list.push_back(p.TR_ptr->BR_ptr);
		}
		else if (p.BR_ptr->TR_ptr != nullptr && p.TR_ptr->BR_ptr != nullptr) {
			if (p.BR_ptr->TR_ptr == p.TR_ptr->BR_ptr) {
				p.R_ptr_list.push_back(p.BR_ptr->TR_ptr);
			}
			else {
				p.R_ptr_list.push_back(p.TR_ptr->BR_ptr);
				p.R_ptr_list.push_back(p.BR_ptr->TR_ptr);
			}
		}
	}

}

int decideElement(ValidPoint *p0, ValidPoint *p1, ValidPoint *p2, ValidPoint *p3) {
	bool solid;
	int sampleSize = 6;// 11;
	//center point
	Vec2d pc;
	if (p2 != nullptr) {
		pc = (p0->position + p2->position) / 2;
	}
	else if (p1 != nullptr && p3 != nullptr) {
		pc = (p1->position + p3->position) / 2;
	}
	else {
		return -1;
	}

	Mat1b centerAreaGray = gray(Rect(round(pc[0] - (sampleSize - 1) / 2), round(pc[1] - (sampleSize - 1) / 2), sampleSize, sampleSize));
	float centerMeanGray = mean(centerAreaGray)[0];
	//mragin
	Vec2f marginEnd;
	if (p1 != nullptr) {
		marginEnd = p1->position;
	}
	else if (p3 != nullptr) {
		marginEnd = p3->position;
	}
	else {
		//dummy
		std::cout << "No margin can be found" << std::endl;
		return - 1;
	}
	LineIterator line_itr(gray, Point(p0->position), Point(marginEnd), 8);
	float marginMean = 0;
	for (int i = 0; i < line_itr.count; i++, ++line_itr) {
		marginMean += gray(line_itr.pos());
	}
	marginMean /= line_itr.count;

	Mat3b sampleArea;
	if (centerMeanGray > marginMean) {
		solid = false;
		//Vec2f pq = (p0->position + pc) / 2;
		Vec2d pq = p0->position + (pc - p0->position) / 2.f;
		//sampleSize = norm((pc - p0->position) / 4.f);
		sampleArea = in_img(Rect(round(pq[0] - (sampleSize - 1) / 2), round(pq[1] - (sampleSize - 1) / 2), sampleSize, sampleSize));
		//circle(temp_img, Point(pq), 3, Scalar(255, 0, 0), -1);
	}
	else {
		solid = true;
		//sampleSize = norm((pc - p0->position) / 2.f);
		sampleArea = in_img(Rect(round(pc[0] - (sampleSize - 1) / 2), round(pc[1] - (sampleSize - 1) / 2), sampleSize, sampleSize));
		//circle(temp_img, Point(pc), 3, Scalar(255, 0, 0), -1);
	}
	Scalar sampleMean = mean(sampleArea);
	int element = decideColor(sampleMean,ColorNamingSpace::HSV_ALT);
	if (!solid)
		element += 4;
	return element;
}
void findElements(ValidPoint &vp) {
	ValidPoint *p0, *p1, *p2, *p3;
	p0 = &vp;

	if (vp.type == 0) {
		//left
		if (vp.L_ptr_list.size() > 0) {
			p2 = vp.L_ptr_list.at(0);
		}
		else {
			p2 = nullptr;
		}
		p1 = vp.BL_ptr;
		p3 = vp.TL_ptr;

		vp.eleLeft = decideElement(p0, p1, p2, p3);
		//right
		if (vp.R_ptr_list.size() > 0) {
			p2 = vp.R_ptr_list.at(0);
		}
		else {
			p2 = nullptr;
		}
		p1 = vp.BR_ptr;
		p3 = vp.TR_ptr;

		vp.eleRight = decideElement(p0, p1, p2, p3);
	}
	if (vp.type == 1) {
		//top
		if (vp.T_ptr_list.size() > 0) {
			p2 = vp.T_ptr_list.at(0);
		}
		else {
			p2 = nullptr;
		}
		p1 = vp.TL_ptr;
		p3 = vp.TR_ptr;

		vp.eleTop = decideElement(p0, p1, p2, p3);
		
		//bottom
		if (vp.B_ptr_list.size() > 0) {
			p2 = vp.B_ptr_list.at(0);
		}
		else {
			p2 = nullptr;
		}
		p1 = vp.BL_ptr;
		p3 = vp.BR_ptr;

		vp.eleBottom = decideElement(p0, p1, p2, p3);
	}
}

void writeVP(std::ofstream& os, ValidPoint& vp) {
	os << vp.type << ",\t" << vp.position << ",\t" << vp.eleLeft << ",\t" << vp.eleRight << ",\t" << vp.eleTop << ",\t" << vp.eleBottom << std::endl;

}
std::vector<ValidPoint> GridPoint2ValidPoint(std::vector<Vec2d> gridPoints, int type) {
	std::vector<ValidPoint> out;
	for (auto _gp = gridPoints.begin(); _gp != gridPoints.end(); ++_gp) {
		ValidPoint _vp;
		_vp.type = type;
		_vp.position = *_gp;
		out.push_back(_vp);
	}
	return out;

}
void decode() {
	//find 4 neighbours
	Mat gridPointsType1 = Mat(detectedGridPointType1).reshape(1); //to 2*n mat
	Mat gridPointsType2 = Mat(detectedGridPointType2).reshape(1); //to 2*n mat
	Mat gridPointsType1_32F, gridPointsType2_32F;
	gridPointsType1.convertTo(gridPointsType1_32F, CV_32F);
	gridPointsType2.convertTo(gridPointsType2_32F, CV_32F);

	//std::vector<ValidPoint> validPoints;
	validPointsType1 = GridPoint2ValidPoint(detectedGridPointType1, 0);
	validPointsType2 = GridPoint2ValidPoint(detectedGridPointType2, 1);

	flann::KDTreeIndexParams indexParams;
	flann::Index kdtree1(gridPointsType1_32F, indexParams);
	flann::Index kdtree2(gridPointsType2_32F, indexParams);
	int K_neighbors = 4;
	for (auto _p = detectedGridPointType1.begin(); _p != detectedGridPointType1.end(); ++_p) {
		//std::cout << *_p << std::endl;
		std::vector<float> query;
		query.push_back((*_p)[0]); //Insert the 2D point we need to find neighbours to the query
		query.push_back((*_p)[1]); //Insert the 2D point we need to find neighbours to the query
		std::vector<int> indices;
		std::vector<float> dists;
		kdtree2.knnSearch(query, indices, dists, K_neighbors);

		//check if the point is surroundered by neighbours
		int topDown = 0, leftRight = 0;
		auto vp = validPointsType1.begin() + std::distance(detectedGridPointType1.begin(), _p);
		for (int i = 0; i < K_neighbors; i++) {
			Vec2f _v = *_p - detectedGridPointType2[indices[i]];
			if (_v[0] > 0 && _v[1] > 0) {
				if (vp->TL_ptr == nullptr) {
					vp->TL_ptr = &validPointsType2[indices[i]];
				}
				else {
					Vec2f _v2 = *_p - vp->TL_ptr->position;
					float d1 = norm(_v, NORM_L2);
					float d2 = norm(_v2, NORM_L2);
					if (d1 < d2)
						vp->TL_ptr = &validPointsType2[indices[i]];
				}
				leftRight++;
				topDown++;
			}
			else if (_v[0] > 0 && _v[1] <= 0) {
				if (vp->BL_ptr == nullptr)
					vp->BL_ptr = &validPointsType2[indices[i]];
				else {
					Vec2f _v2 = *_p - vp->BL_ptr->position;
					float d1 = norm(_v, NORM_L2);
					float d2 = norm(_v2, NORM_L2);
					if (d1 < d2)
						vp->BL_ptr = &validPointsType2[indices[i]];
				}
				leftRight++;
				topDown--;
			}
			else if (_v[0] <= 0 && _v[1] > 0) {
				if (vp->TR_ptr == nullptr)
					vp->TR_ptr = &validPointsType2[indices[i]];
				else {
					Vec2f _v2 = *_p - vp->TR_ptr->position;
					float d1 = norm(_v, NORM_L2);
					float d2 = norm(_v2, NORM_L2);
					if (d1 < d2)
						vp->TR_ptr = &validPointsType2[indices[i]];
				}
				leftRight--;
				topDown++;
			}
			else if (_v[0] <= 0 && _v[1] <= 0) {
				if (vp->BR_ptr == nullptr)
					vp->BR_ptr = &validPointsType2[indices[i]];
				else {
					Vec2f _v2 = *_p - vp->BR_ptr->position;
					float d1 = norm(_v, NORM_L2);
					float d2 = norm(_v2, NORM_L2);
					if (d1 < d2)
						vp->BR_ptr = &validPointsType2[indices[i]];
				}
				leftRight--;
				topDown--;
			}
		}

		//if (topDown != 0 || leftRight != 0) continue;
		//else consider it as a valid point;
		//vp.type = 0;
		//vp.idxC = std::distance(detectedGridPointType1.begin(), _p);
		//validPoints.push_back(vp);
		//suppose a point must be inside 4 neighbours of the different type, do a pointPolygonTest
		/*std::vector<Vec2f> polygon;
		for (int i = 0; i < 4; i++) {
			polygon.push_back(detectedGridPointType2[indices[i]]);
		}
		int in_poly = pointPolygonTest(polygon, Point2f((*_p)[0], (*_p)[1]), false);

		if (in_poly <= 0) continue;*/

		//visulization
		/*Mat temp_img;
		in_img.copyTo(temp_img);
		circle(temp_img, Point((int)(*_p)[0], (int)(*_p)[1]), 10, Scalar(0, 0, 255), -1);
		for (int i = 0; i < 4; i++) {
			Vec2f neighbour = detectedGridPointType2[indices[i]];
			circle(temp_img, Point(neighbour[0], neighbour[1]), 5, Scalar(255, 0, 0), -1);
		}
		imshow("knn test", temp_img);
		waitKey();*/

	}
	for (auto _p = detectedGridPointType2.begin(); _p != detectedGridPointType2.end(); ++_p) {
		//std::cout << *_p << std::endl;
		std::vector<float> query;
		query.push_back((*_p)[0]); //Insert the 2D point we need to find neighbours to the query
		query.push_back((*_p)[1]); //Insert the 2D point we need to find neighbours to the query
		std::vector<int> indices;
		std::vector<float> dists;
		kdtree1.knnSearch(query, indices, dists, K_neighbors);

		//check if the point is surroundered by neighbours
		int topDown = 0, leftRight = 0;
		auto vp = validPointsType2.begin() + std::distance(detectedGridPointType2.begin(), _p);
		for (int i = 0; i < K_neighbors; i++) {
			Vec2f _v = *_p - detectedGridPointType1[indices[i]];
			if (_v[0] > 0 && _v[1] > 0) {
				if (vp->TL_ptr == nullptr) {
					vp->TL_ptr = &validPointsType1[indices[i]];
				}
				else {
					Vec2f _v2 = *_p - vp->TL_ptr->position;
					float d1 = norm(_v, NORM_L2);
					float d2 = norm(_v2, NORM_L2);
					if (d1 < d2)
						vp->TL_ptr = &validPointsType1[indices[i]];
				}
				leftRight++;
				topDown++;
			}
			else if (_v[0] > 0 && _v[1] <= 0) {
				if (vp->BL_ptr == nullptr)
					vp->BL_ptr = &validPointsType1[indices[i]];
				else {
					Vec2f _v2 = *_p - vp->BL_ptr->position;
					float d1 = norm(_v, NORM_L2);
					float d2 = norm(_v2, NORM_L2);
					if (d1 < d2)
						vp->BL_ptr = &validPointsType1[indices[i]];
				}
				leftRight++;
				topDown--;
			}
			else if (_v[0] <= 0 && _v[1] > 0) {
				if (vp->TR_ptr == nullptr)
					vp->TR_ptr = &validPointsType1[indices[i]];
				else {
					Vec2f _v2 = *_p - vp->TR_ptr->position;
					float d1 = norm(_v, NORM_L2);
					float d2 = norm(_v2, NORM_L2);
					if (d1 < d2)
						vp->TR_ptr = &validPointsType1[indices[i]];
				}
				leftRight--;
				topDown++;
			}
			else if (_v[0] <= 0 && _v[1] <= 0) {
				if (vp->BR_ptr == nullptr)
					vp->BR_ptr = &validPointsType1[indices[i]];
				else {
					Vec2f _v2 = *_p - vp->BR_ptr->position;
					float d1 = norm(_v, NORM_L2);
					float d2 = norm(_v2, NORM_L2);
					if (d1 < d2)
						vp->BR_ptr = &validPointsType1[indices[i]];
				}
				leftRight--;
				topDown--;
			}
		}
		//if (topDown != 0 || leftRight != 0) continue;
		//else consider it as a valid point;
		//vp.type = 1;
		//vp.idxC = std::distance(detectedGridPointType2.begin(), _p);
		//validPoints.push_back(vp);
	}

	
	std::ofstream os;
	os.open("./"+ outFilePrefix+"_validpoints.csv");
	for (auto _vp = validPointsType1.begin(); _vp != validPointsType1.end(); ++_vp) {
		findTBLR(*_vp);
		findElements(*_vp);
		writeVP(os, *_vp);
	}
	for (auto _vp = validPointsType2.begin(); _vp != validPointsType2.end(); ++_vp) {
		findTBLR(*_vp);
		findElements(*_vp);
		writeVP(os, *_vp);
	}

	os.close();

	//Find codewords
	for (auto _vp = validPointsType1.begin(); _vp != validPointsType1.end(); ++_vp) {
		Vec4i codeword;
		if (_vp->eleLeft >= 0 && _vp->eleRight >= 0) {
			for (size_t i = 0; i < _vp->B_ptr_list.size(); i++) {
				if (_vp->B_ptr_list[i]->eleLeft >= 0 && _vp->B_ptr_list[i]->eleRight >= 0) {
					codeword = Vec4i(_vp->eleLeft, _vp->eleRight, _vp->B_ptr_list[i]->eleLeft, _vp->B_ptr_list[i]->eleRight);
					_vp->codewords.push_back(codeword);
				}
			}
		}
	}
	for (auto _vp = validPointsType2.begin(); _vp != validPointsType2.end(); ++_vp) {
		Vec4i codeword;
		if (_vp->eleTop >= 0 && _vp->eleBottom >= 0) {
			for (size_t i = 0; i < _vp->R_ptr_list.size(); i++) {
				if (_vp->R_ptr_list[i]->eleTop >= 0 && _vp->R_ptr_list[i]->eleBottom >= 0) {
					codeword = Vec4i(_vp->eleTop, _vp->R_ptr_list[i]->eleTop, _vp->eleBottom, _vp->R_ptr_list[i]->eleBottom);
					_vp->codewords.push_back(codeword);
				}
			}
		}
	}

	//show feature positions
	for (auto _vp = validPointsType1.begin(); _vp != validPointsType1.end(); ++_vp) {
		Scalar color;
		Vec2f _p;
		if (_vp->type == 0) {
			color = Scalar(0, 0, 255);
			_p = _vp->position;
			circle(temp_img, Point(_p[0], _p[1]), 5, color, -1);
		}
	}
	for (auto _vp = validPointsType2.begin(); _vp != validPointsType2.end(); ++_vp) {
		Scalar color;
		Vec2f _p;
		if (_vp->type == 1) {
			color = Scalar(255, 0, 0);
			_p = _vp->position;
			circle(temp_img, Point(_p[0], _p[1]), 5, color, -1);
		}
	}
	imshow("valid points", temp_img);

	//save camera features to mat
	std::vector<Vec2d> camFeatsType1, camFeatsType2, camFeatsAll;
	//std::vector<KeyPoint> camKeyPointsType1, camKeyPointsType2;
	for (size_t i = 0; i < validPointsType1.size(); i++) {
		camFeatsType1.push_back(validPointsType1[i].position * scale);
		camFeatsAll.push_back(validPointsType1[i].position * scale);
	}
	for (size_t i = 0; i < validPointsType2.size(); i++) {
		camFeatsType2.push_back(validPointsType2[i].position * scale);
		camFeatsAll.push_back(validPointsType2[i].position * scale);
	}


	FileStorage camFeatsFs(outFilePrefix + "_CameraFeatures.yml", FileStorage::WRITE);
	camFeatsFs << "type1" << camFeatsType1;
	camFeatsFs << "type2" << camFeatsType2;

	FileStorage camFeatsAllFs(outFilePrefix + "_CameraFeaturesAll.yml", FileStorage::WRITE);
	camFeatsAllFs << "features" << camFeatsAll;
	
	std::cout << "Detected " << camFeatsType1.size() << " camera feats type1." << std::endl;
	std::cout << "Detected " << camFeatsType2.size() << " camera feats type2." << std::endl;
	std::cout << "Detected " << camFeatsAll.size() << " camera feats in total." << std::endl;
	waitKey();

}

int main(int argc, char **argv) {

	CmdLine cmd;
	
	int gaussianBlurSize = 0;
	cmd.add(make_option('i', imgFileName, "imgFileName"));
	cmd.add(make_option('o', outFilePrefix, "outFilePrefix"));
	cmd.add(make_option('m', MASK_SIZE, "maskSize"));
	cmd.add(make_option('g', gaussianBlurSize, "gaussianBlurSize"));
	cmd.add(make_option('s', scale, "scale factor"));
	try {
		if (argc == 1)
		{
			imgFileName =  "./cam7.jpg";
			outFilePrefix = "";
		}
		else {
			cmd.process(argc, argv);
		}
	}
	catch (const std::string& s)
	{
		std::cout << "Invalid usage of arguments -i ImgFileName -o OutputFilePrefix -m maskSize" << std::endl;
		return EXIT_FAILURE;
	}
	//String in_path = "./pattern.png";
	//String in_path = "./cam7.jpg";
	in_img = imread(imgFileName);
	resize(in_img, in_img, Size(), 1.f / scale, 1.f / scale);
	in_img.copyTo(temp_img);
	
	cvtColor(in_img, gray, CV_BGR2GRAY);
	Mat1b blur_gray;
	Mat3b blur_color;
	if (gaussianBlurSize > 0) {
		GaussianBlur(in_img, blur_color, Size(gaussianBlurSize, gaussianBlurSize), 0);
		GaussianBlur(gray, blur_gray, Size(gaussianBlurSize, gaussianBlurSize), 0);
	}
	else {
		blur_color = in_img.clone();
		blur_gray = gray.clone();
	}
	//threshold(gray, gray, 200, 255, ThresholdTypes::THRESH_BINARY);

	imshow("gray", gray);
	imshow("gray blur", blur_gray);
	imshow("color blur", blur_color);

	Mat gp;
	CandidateGP(blur_color, gp);
	RefineGP(blur_gray, gp, "PMCC");

	decode();

	C2PMatching(validPointsType1,validPointsType2, in_img);

	waitKey();

	return 0;
}


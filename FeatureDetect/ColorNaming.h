#pragma once

#include "opencv.hpp"

using namespace cv;

enum ColorNamingSpace
{
	HSV,
	LAB,
	HSV_ALT
};
int decideColor(Scalar & color, ColorNamingSpace colorSpace = HSV) {

	if (colorSpace == LAB) {
		Mat bgr(1, 1, CV_8UC3, color);
		//Vec3b bgr(color[0], color[1], color[2]);
		Mat3b lab;
		cvtColor(bgr, lab, COLOR_BGR2Lab);
		Mat3b r(Vec3b(0, 0, 255)),
			g(Vec3b(0, 255, 0)),
			b(Vec3b(255, 0, 0)),
			k(Vec3b(0, 0, 0));

		Mat3b r_lab, g_lab, b_lab, k_lab;
		
		cvtColor(r, r_lab, COLOR_BGR2Lab);
		cvtColor(g, g_lab, COLOR_BGR2Lab);
		cvtColor(b, b_lab, COLOR_BGR2Lab);
		cvtColor(k, k_lab, COLOR_BGR2Lab);
		int d_r = (r_lab - lab).dot(r_lab - lab);
		int d_g = (g_lab - lab).dot(g_lab - lab);
		int d_b = (b_lab - lab).dot(b_lab - lab);
		int d_k = (k_lab - lab).dot(k_lab - lab);
		int d_min = min(d_r, min(d_g, min(d_b, d_k)));
		if (d_min == d_r) {
			return 0;
		}
		else if (d_min == d_g) {
			return 1;
		}
		else if (d_min == d_b) {
			return 2;
		}
		else if (d_min == d_k) {
			return 3;
		}
		else return -1;

	}
	if (colorSpace == HSV) {
		Mat bgr(1, 1, CV_32FC3, color / 255.f);
		Mat hsv;
		cvtColor(bgr, hsv, COLOR_BGR2HSV);
		float h = hsv.at<Vec3f>(0, 0)[0];
		float s = hsv.at<Vec3f>(0, 0)[1];
		float v = hsv.at<Vec3f>(0, 0)[2];
		if (v < 0.6) { //black
			return 3;
		}
		else if ( s < 0.05) { //black??
			return 3;
		}
		else if (h < 60 || h >= 300) {//red
			return 0;
		}
		else if (h >= 60 && h < 180) {//green
			return 1;
		}
		else if (h >= 180 && h < 300) {//blue
			return 2;
		}
		else {
			std::cout << " color detect failed " << std::endl;
			return -1;
		}
	}
	if (colorSpace == HSV_ALT) {

		float b = color[0], g = color[1], r = color[2];

		float s = sqrt(1 - (r*g + g * b + r * b) / (r*r + g * g + b * b));

		float hr, hg, hb;
		float temp;
		//hr
		temp = (r - g)*(r - g) + (r - b)*(g - b);
		if (temp <= 0)
			hr = 0;
		else
			hr = (2 * r - g - b) / (2 * sqrt(temp));

		//hg
		temp = (g - r)*(g - r) + (g - b)*(r - b);
		if (temp <= 0)
			hg = 0;
		else
			hg = (2 * g - r - b) / (2 * sqrt(temp));

		//hb
		temp = (b - g)*(b - g) + (b - r)*(g - r);
		if (temp <= 0)
			hb = 0;
		else
			hb = (2 * b - g - r) / (2 * sqrt(temp));

		float hmax = max(max(hr, hg), hb);
		float k = s - sqrt(1 - hmax * hmax);
		if (k < 0.02)//black
			return 3;
		else if (hmax == hr)
			return 0;
		else if (hmax == hg)
			return 1;
		else if (hmax == hb)
			return 2;
		else {
			std::cout << " color detect failed " << std::endl;
			return -1;
		}
	}

}
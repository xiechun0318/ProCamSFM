/*
  *********************************************************************
  *                                                                   *
  *               Galois Field Arithmetic Library                     *
  * Prototype: Galois Field STD OUT Test                              *
  * Version: 0.0.1                                                    *
  * Author: Arash Partow - 2000                                       *
  * URL: http://www.partow.net/projects/galois/index.html             *
  *                                                                   *
  * Copyright Notice:                                                 *
  * Free use of this library is permitted under the guidelines and    *
  * in accordance with the most current version of the Common Public  *
  * License.                                                          *
  * http://www.opensource.org/licenses/cpl.php                        *
  *                                                                   *
  *********************************************************************
*/



#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include "GaloisField.h"
#include "GaloisFieldElement.h"
#include "GaloisFieldPolynomial.h"

#include "opencv.hpp"


/*
   p(x) = 1x^8+1x^7+0x^6+0x^5+0x^4+0x^3+1x^2+1x^1+1x^0
		  1    1    0    0    0    0    1    1    1
*/
//unsigned int poly[9] = {1,1,1,0,0,0,0,1,1};
unsigned int poly[4] = { 1,1,0,1 };

/*
  A Galois Field of type GF(2^8)
*/

galois::GaloisField gf(3, poly);

bool checkWindowProperty(cv::Mat arr) {
	bool res = true;
	for (int i = 0; i < arr.rows - 1; i++) {
		for (int j = 0; j < arr.cols - 1; j++) {
			cv::Mat1i roi1 = arr(cv::Rect(j, i, 2, 2));

			//std::cout << roi1 << std::endl;

			for (int k = 0; k < arr.rows - 1; k++) {
				for (int l = 0; l < arr.cols - 1; l++) {
					if (i == k && j == l) continue;
					cv::Mat1i roi2 = arr(cv::Rect(l, k, 2, 2));

					//std::cout << roi2 << std::endl;

					cv::Mat diff = roi1 != roi2;
					if (cv::countNonZero(diff) == 0) {
						std::cout << "Window property error: (" << j << "," << i << ") (" << l << "," << k << ")" << std::endl;
						std::cout << roi1 << std::endl;
						std::cout << roi2 << std::endl;
						res = false;
					}

				}
			}

		}

	}
	return res;
}
void createPattern(const cv::Mat1i& labels, cv::Mat& img) {
	int elmSize = 11;
	img = cv::Mat3b(labels.size()*11,cv::Vec3b(255,255,255));
	for (int i = 0; i < labels.rows; i++) {
		for (int j = 0; j < labels.cols; j++) {
			int label = labels(i, j);
			cv::Point oriPoint(j*elmSize, i*elmSize);
			cv::Scalar color;
			int colorLabel = label % 4;
			if (colorLabel == 0)//R
				color = cv::Scalar(0, 0, 255);
			else if (colorLabel == 1)//G
				color = cv::Scalar(0, 255, 0);
			else if (colorLabel == 3)//B
				color = cv::Scalar(255, 0, 0);
			else if (colorLabel == 4)//K
				color = cv::Scalar(0, 0, 0);

			//draw solid
			std::vector<cv::Point> pts;
			pts.push_back(cv::Point((elmSize - 1) / 2, 0) + oriPoint);
			pts.push_back(cv::Point(0, (elmSize - 1) / 2) + oriPoint);
			pts.push_back(cv::Point((elmSize - 1) / 2, elmSize - 1) + oriPoint);
			pts.push_back(cv::Point(elmSize - 1, (elmSize - 1) / 2) + oriPoint);

			cv::fillConvexPoly(img, pts, color, cv::LINE_8);

			if (label >= 4) {// non-solid
				std::vector<cv::Point> pts_w;
				pts_w.push_back(cv::Point((elmSize - 1) / 2, (elmSize - 1) / 4 + 1) + oriPoint);
				pts_w.push_back(cv::Point((elmSize - 1) / 4 + 1, (elmSize - 1) / 2) + oriPoint);
				pts_w.push_back(cv::Point((elmSize - 1) / 2, elmSize - 1  - (elmSize - 1) / 4 - 1) + oriPoint);
				pts_w.push_back(cv::Point(elmSize - 1 - (elmSize - 1) / 4 - 1, (elmSize - 1) / 2) + oriPoint);
				cv::fillConvexPoly(img, pts_w, cv::Scalar(255, 255, 255), cv::LINE_8);
			}
		}
	}
}
int main(int argc, char *argv[])
{

	std::cout << gf << std::endl;
	galois::GaloisFieldElement element0(&gf, 0);
	galois::GaloisFieldElement element1(&gf, 1);
	galois::GaloisFieldElement element2(&gf, 2);
	galois::GaloisFieldElement element3(&gf, 3);
	galois::GaloisFieldElement element4(&gf, 4);
	galois::GaloisFieldElement element5(&gf, 5);
	galois::GaloisFieldElement element6(&gf, 6);
	galois::GaloisFieldElement element7(&gf, 7);

	std::cout << element0 << std::endl;
	std::cout << element1 << std::endl;
	std::cout << element2 << std::endl;
	std::cout << element3 << std::endl;
	std::cout << element4 << std::endl;
	std::cout << element5 << std::endl;
	std::cout << element6 << std::endl;
	std::cout << element7 << std::endl;


	std::vector<galois::GaloisFieldElement> seq;
	seq.push_back(element1);
	seq.push_back(element0);
	seq.push_back(element1);
	seq.push_back(element0);
	//seq.push_back(element1);
	//seq.push_back(element1);
	//seq.push_back(element4);
	for (int i = 0; i < 4095 - 4; i++) {
		galois::GaloisFieldElement e = seq[i] * element3 + seq[i + 1];
		seq.push_back(e);
	}

	for (auto i = seq.begin(); i != seq.end(); ++i)
		std::cout << i->index() + 1 << ' ';

	std::cout << "sequence size = " << seq.size() << std::endl;

	cv::Mat1i arr(63, 65, -1);

	//std::cout << arr << std::endl;

	int row = 0;
	int col = 0;
	for (int i = 0; i < 4095; i++) {
		if (arr(row, col) != -1) {
			std::cout << "Array generation error: " << row << "," << col << std::endl;
		}
		arr(row, col) = seq.at(i).index() + 1;
		row = (row + 1) % 63;
		col = (col + 1) % 65;
	}

	std::cout << arr << std::endl;
	std::cout << "Finished" << std::endl;

	cv::Mat3b pattern;
	createPattern(arr, pattern);
	cv::imshow("pattern", pattern);

	cv::waitKey();
	exit(EXIT_SUCCESS);
	return true;

}


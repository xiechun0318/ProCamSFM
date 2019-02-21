#include "stdafx.h"
#include "C2PMatching.h"
#include "global.h"

void C2PMatching(const std::vector<ValidPoint> vpType1, const std::vector<ValidPoint> vpType2, const Mat &_camImg)
{
	FileStorage projFeatsAllfs("ProjectorFeaturesAll.yml", FileStorage::READ);
	if (!projFeatsAllfs.isOpened()) {
		std::cout << "File can not be opened." << std::endl;
		return;
	}
	std::vector<cv::Vec2d> projFeats;
	std::vector< cv::Vec4i> projFeatCodes;
	
	projFeatsAllfs["features"] >> projFeats;
	projFeatsAllfs["codewords"] >> projFeatCodes;

	std::cout << "read " << projFeats.size() << " projector feats point." << std::endl;
	std::cout << "read " << projFeatCodes.size() << " projector feat codewords." << std::endl;

	//find matches
	std::vector<Vec2i> matchesType1, matchesType2, matchesAll;
	//type 1
	for (size_t k = 0; k < vpType1.size(); k++) {
		for (size_t c = 0; c < vpType1[k].codewords.size(); c++) {
			for (size_t i = 0; i < projFeatCodes.size() / 2; i++) {
				if (vpType1[k].codewords[c] == projFeatCodes[i]) {
					int projIdx = i;
					matchesType1.push_back(Vec2i(k, projIdx));
					matchesAll.push_back(Vec2i(k, projIdx));
				}
			}
		}
	}
	//type 2
	for (size_t k = 0; k < vpType2.size(); k++) {
		for (size_t c = 0; c < vpType2[k].codewords.size(); c++) {
			for (size_t i = projFeatCodes.size() / 2; i < projFeatCodes.size(); i++) {
				if (vpType2[k].codewords[c] == projFeatCodes[i]) {
					int projIdx = i;
					matchesType2.push_back(Vec2i(k, projIdx - projFeatCodes.size() / 2));
					matchesAll.push_back(Vec2i(k + vpType1.size(), projIdx));				
				}
			}
		}
	}

	FileStorage matchesFs(outFilePrefix + "_Matches.yml", FileStorage::WRITE);
	matchesFs << "matches" << matchesAll;

	std::cout << "Detected " << matchesType1.size() << " matches type1." << std::endl;
	std::cout << "Detected " << matchesType2.size() << " matches type2." << std::endl;
	std::cout << "Detected " << matchesAll.size() << " matches in total." << std::endl;

	//visulize matches
	std::vector<cv::Vec2d> projFeatsType1(projFeats.begin(), projFeats.begin() + (projFeats.size() / 2)),
		projFeatsType2(projFeats.begin() + (projFeats.size() / 2), projFeats.end());

	float zoomFactor = 5;
	std::vector<KeyPoint> projKeyPointsType1 = Vec2d2KeyPoint(projFeatsType1, zoomFactor),
		projKeyPointsType2 = Vec2d2KeyPoint(projFeatsType2, zoomFactor);

	std::vector<KeyPoint> camKeyPointsType1 = ValidPoint2KeyPoint(vpType1), 
		camKeyPointsType2 = ValidPoint2KeyPoint(vpType2);
	
	std::vector<DMatch> DmatchesType1 = Vec2i2Dmatch(matchesType1),
		DmatchesType2 = Vec2i2Dmatch(matchesType2);

	std::vector<DMatch> subDmatchesType1, subDmatchesType2;
	
	int subsample_step = 5;
	for (int i = 0; i < DmatchesType1.size(); i += subsample_step) {
		subDmatchesType1.push_back(DmatchesType1[i]);
	}
	for (int i = 0; i < DmatchesType2.size(); i += subsample_step) {
		subDmatchesType2.push_back(DmatchesType2[i]);
	}

	Mat camImg = _camImg;
	Mat projImg = imread("pattern.tiff");
	resize(projImg, projImg, Size(), zoomFactor, zoomFactor);
	Mat matchImg1, matchImg2;

	drawMatches(camImg, camKeyPointsType1, projImg, projKeyPointsType1, subDmatchesType1, matchImg1);
	drawMatches(camImg, camKeyPointsType2, projImg, projKeyPointsType2, subDmatchesType2, matchImg2);

	imshow("matchImg1", matchImg1);
	imshow("matchImg2", matchImg2);
	waitKey();
}

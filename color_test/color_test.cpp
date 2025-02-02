// color_test.cpp : このファイルには 'main' 関数が含まれています。プログラム実行の開始と終了がそこで行われます。
//

#include "pch.h"
#include <iostream>

using namespace cv;

int main(int argc, char **argv)
{
	CmdLine cmd;

	String imgFileName;
	cmd.add(make_option('i', imgFileName, "imgFileName"));

	try {
		if (argc == 1)
		{
			imgFileName = "./cam0.jpg";
		}
		else {
			cmd.process(argc, argv);
		}
	}
	catch (const std::string& s)
	{
		std::cout << "Invalid usage of arguments -i ImgFileName -o OutputFilePrefix" << std::endl;
		return EXIT_FAILURE;
	}
	
	Mat in_img = imread(imgFileName);

	//cvtColor(in_img, gray, CV_bgr2gray);
	//GaussianBlur(gray, gray, Size(3, 3), 0);
	//threshold(gray, gray, 200, 255, ThresholdTypes::THRESH_BINARY);

	//imshow("gray", gray);
	
}

// プログラムの実行: Ctrl + F5 または [デバッグ] > [デバッグなしで開始] メニュー
// プログラムのデバッグ: F5 または [デバッグ] > [デバッグの開始] メニュー

// 作業を開始するためのヒント: 
//    1. ソリューション エクスプローラー ウィンドウを使用してファイルを追加/管理します 
//   2. チーム エクスプローラー ウィンドウを使用してソース管理に接続します
//   3. 出力ウィンドウを使用して、ビルド出力とその他のメッセージを表示します
//   4. エラー一覧ウィンドウを使用してエラーを表示します
//   5. [プロジェクト] > [新しい項目の追加] と移動して新しいコード ファイルを作成するか、[プロジェクト] > [既存の項目の追加] と移動して既存のコード ファイルをプロジェクトに追加します
//   6. 後ほどこのプロジェクトを再び開く場合、[ファイル] > [開く] > [プロジェクト] と移動して .sln ファイルを選択します

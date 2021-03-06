#include "mnist.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <algorithm>
#include <Eigen/Dense>

namespace MyDL
{

    using std::cout;
    using std::endl;
    using std::ifstream;
    using std::ios;
    using std::string;
    using std::vector;
    using namespace Eigen;

    // リトルエンディアン → ビッグエンディアンへの変換
    int LittleEndian2BigEndian(int i)
    {
        unsigned char c1, c2, c3, c4;

        c1 = i & 255;         // 0x 00 00 00 xx (xxを抽出, 255 = 0x 00 00 00 FF)
        c2 = (i >> 8) & 255;  // 0x 00 00 xx 00 (xxを抽出 ※下8桁は右シフトで消える)
        c3 = (i >> 16) & 255; // 0x 00 xx 00 00 (xxを抽出 ※下16桁は右シフトで消える)
        c4 = (i >> 24) & 255; // 0x xx 00 00 00 (xxを抽出 ※下24桁は右シフトで消える)

        return ((int)c1 << 24) + ((int)c2 << 16) + ((int)c3 << 8) + ((int)c4);
    }

    // ------------------------------------------------------
    //              Eigen用 MNISTローダ 実装
    // ------------------------------------------------------

    // コンストラクタ
    MnistEigenDataset::MnistEigenDataset(int batch_size, bool random_load)
    {
        _batch_size = batch_size;

        // ファイル読み込み初期化処理
        _init_train_loader();
        _init_test_loader();

        _train_max_batch_num = (_number_of_train_data + _batch_size - 1) / _batch_size; // 切り上げ
        _test_max_batch_num = (_number_of_test_data + _batch_size - 1) / _batch_size;

        // ランダム読み出しの設定をしているときはインデックスをシャッフル
        if (random_load)
        {
            std::mt19937_64 get_rand_mt;
            std::shuffle(_train_indices.begin(), _train_indices.end(), get_rand_mt);
            std::shuffle(_test_indices.begin(), _test_indices.end(), get_rand_mt);
        }
    }


    void MnistEigenDataset::next_train(MatrixXd &train_X, MatrixXd &train_y, bool one_hot_label, bool normalize)
    {
        // 読み出し用一時変数
        int pixels = _rows * _cols;
        vector<double> tmp_image(pixels); // 配列のサイズ初期化が必要(でないとセグフォ)
        vector<double> tmp_labels(_batch_size);

        if (one_hot_label)
        {
            train_y = MatrixXd::Zero(_batch_size, 10); // one_hot_label有効化時の初期化
        }

        // インデックス取得：初期位置計算
        int start_idx = _batch_size * _train_load_count;
        int tmp_idx;
        // ここに必要なだけ訓練データを読み出す処理を作成
        for (int i = 0; i < _batch_size; i++)
        {
            // 59999を超えたインデックスは0から再カウント
            tmp_idx = _train_indices[(start_idx + i) % _number_of_train_data];

            // ファイルシーク：画像データのインターバルは「28×28=784byte」あるので注意
            // 画像データ
            _train_image_ifs.seekg(_train_image_pos);                     // シークを初期位置に
            _train_image_ifs.seekg(tmp_idx * pixels, std::ios_base::cur); // 読み出し位置まで移動
            // ラベル
            _train_label_ifs.seekg(_train_label_pos);                     // シークを初期位置に
            _train_label_ifs.seekg(tmp_idx, std::ios_base::cur);          // 読み出し位置まで移動

            // 画像読み出し
            for (int j = 0; j < pixels; j++)
            {
                unsigned char tmp_value;
                _train_image_ifs.read((char *)&tmp_value, sizeof(tmp_value));
                tmp_image[j] = (double)(tmp_value);
            }

            train_X.row(i) = Map<Matrix<double, 1, 28 * 28>>(&(tmp_image[0]));

            // ラベル読み出し
            unsigned char tmp_label;
            _train_label_ifs.read((char *)&tmp_label, sizeof(tmp_label));
            tmp_labels[i] = (double)(tmp_label);

            // one-hotか否かで場合分け
            if (one_hot_label)
            {
                train_y(i, int(tmp_label)) = 1;
            }
            else
            {
                train_y.row(i) = Map<Matrix<double, 1, 1>>(&(tmp_labels[i]));
            }
        }

        if (normalize)
        {
            train_X /= 255;
        }

        _train_load_count++;
        // カウンタリセット → バッチ数とカウントが同じになったら0にする
        _train_load_count = _train_load_count % _train_max_batch_num;
    }


    void MnistEigenDataset::next_test(MatrixXd &test_X, MatrixXd &test_y, bool one_hot_label, bool normalize)
    {

        // 読み出し用一時変数
        int pixels = _rows * _cols;
        vector<double> tmp_image(pixels);
        vector<double> tmp_labels(_batch_size);

        if (one_hot_label)
        {
            test_y = MatrixXd::Zero(_batch_size, 10);
        }

        // インデックス取得：初期位置計算
        int start_idx = _batch_size * _test_load_count;
        int tmp_idx;
        // ここに必要なだけ訓練データを読み出す処理を作成
        for (int i = 0; i < _batch_size; i++)
        {
            // 59999を超えたインデックスは0から再カウント
            tmp_idx = _test_indices[(start_idx + i) % (_number_of_test_data)];
            // ファイルシーク：画像データのインターバルは「28×28=784byte」あるので注意
            // 画像データ
            _test_image_ifs.seekg(_test_image_pos);                      // シークを初期位置に
            _test_image_ifs.seekg(tmp_idx * pixels, std::ios_base::cur); // 読み出し位置まで移動
            // ラベル
            _test_label_ifs.seekg(_test_label_pos);                      // シークを初期位置に
            _test_label_ifs.seekg(tmp_idx, std::ios_base::cur);          // 読み出し位置まで移動

            // 画像読み出し
            for (int j = 0; j < pixels; j++)
            {
                unsigned char tmp_value;
                _test_image_ifs.read((char *)&tmp_value, sizeof(tmp_value));
                tmp_image[j] = (double)(tmp_value);
            }
            test_X.row(i) = Map<Matrix<double, 1, 28 * 28>>(&(tmp_image[0]));

            // ラベル読み出し
            unsigned char tmp_label;
            _test_label_ifs.read((char *)&tmp_label, sizeof(tmp_label));
            tmp_labels[i] = (double)(tmp_label);

            // one-hotか否かで場合分け
            if (one_hot_label)
            {
                test_y(i, int(tmp_label)) = 1;
            }
            else
            {
                test_y.row(i) = Map<Matrix<double, 1, 1>>(&(tmp_labels[i]));
            }
        }

        if (normalize)
        {
            test_X /= 255;
        }

        _test_load_count++;
        // カウンタリセット → バッチ数とカウントが同じになったら0にする
        _test_load_count = _test_load_count % _test_max_batch_num;
    }

    // ファイルパス setter
    void MnistEigenDataset::set_train_image_filepath(string filepath)
    {
        _train_image_filepath = filepath;
    }

    void MnistEigenDataset::set_train_label_filepath(string filepath)
    {
        _train_label_filepath = filepath;
    }

    void MnistEigenDataset::set_test_image_filepath(string filepath)
    {
        _test_image_filepath = filepath;
    }

    void MnistEigenDataset::set_test_label_filepath(string filepath)
    {
        _test_label_filepath = filepath;
    }

    // パス設定後の初期化処理
    void MnistEigenDataset::initialize_loader(void)
    {
        _init_test_loader();
        _init_train_loader();
    }

    // -------------------------------------------------------------
    //                   内部メソッド
    // -------------------------------------------------------------
    void MnistEigenDataset::_init_train_loader(void)
    {
        // 設定したファイルパスに基づいてファイルオープン
        _train_image_ifs.open(_train_image_filepath, std::ios::in | std::ios::binary);
        _train_label_ifs.open(_train_label_filepath, std::ios::in | std::ios::binary);
        int magic_number = 0;

        // 適切な位置までファイルポインタ移動
        _train_image_ifs.read((char *)&magic_number, sizeof(magic_number)); // sizeof(magic_number) = 4, つまり4byte分読み出し
        magic_number = LittleEndian2BigEndian(magic_number);
        _train_image_ifs.read((char *)&_number_of_train_data, sizeof(_number_of_train_data));
        _number_of_train_data = LittleEndian2BigEndian(_number_of_train_data);
        _train_image_ifs.read((char *)&_rows, sizeof(_rows));
        _rows = LittleEndian2BigEndian(_rows);
        _train_image_ifs.read((char *)&_cols, sizeof(_cols));
        _cols = LittleEndian2BigEndian(_cols);

        cout << "IMAGE magic number: " << magic_number << endl;

        // 適切な位置までファイルポインタ移動
        _train_label_ifs.read((char *)&magic_number, sizeof(magic_number));
        magic_number = LittleEndian2BigEndian(magic_number);
        _train_label_ifs.read((char *)&_number_of_train_data, sizeof(_number_of_train_data));
        _number_of_train_data = LittleEndian2BigEndian(_number_of_train_data);

        cout << "LABEL magic number: " << magic_number << endl;

        // ファイルポインタ：シーク位置の記憶
        _train_image_pos = _train_image_ifs.tellg();
        _train_label_pos = _train_label_ifs.tellg();

        // 読み出しのためのインデックス作成：単なる整数型でOK(int)
        for (int i = 0; i < _number_of_train_data; i++)
        {
            _train_indices.push_back(i);
        }
    }

    void MnistEigenDataset::_init_test_loader(void)
    {
        // 設定したファイルパスに基づいてファイルオープン
        _test_image_ifs.open(_test_image_filepath, std::ios::in | std::ios::binary);
        _test_label_ifs.open(_test_label_filepath, std::ios::in | std::ios::binary);
        int magic_number = 0;

        // 適切な位置までディスクリプタ移動
        _test_image_ifs.read((char *)&magic_number, sizeof(magic_number)); // sizeof(magic_number) = 4, つまり4byte分読み出し
        magic_number = LittleEndian2BigEndian(magic_number);
        _test_image_ifs.read((char *)&_number_of_test_data, sizeof(_number_of_test_data));
        _number_of_test_data = LittleEndian2BigEndian(_number_of_test_data);
        _test_image_ifs.read((char *)&_rows, sizeof(_rows));
        _rows = LittleEndian2BigEndian(_rows);
        _test_image_ifs.read((char *)&_cols, sizeof(_cols));
        _cols = LittleEndian2BigEndian(_cols);

        cout << "IMAGE magic number: " << magic_number << endl;

        // 適切な位置までディスクリプタ移動
        _test_label_ifs.read((char *)&magic_number, sizeof(magic_number));
        magic_number = LittleEndian2BigEndian(magic_number);
        _test_label_ifs.read((char *)&_number_of_test_data, sizeof(_number_of_test_data));
        _number_of_test_data = LittleEndian2BigEndian(_number_of_test_data);

        cout << "LABEL magic number: " << magic_number << endl;

        // ファイルポインタ：シーク位置の記憶
        _test_image_pos = _test_image_ifs.tellg();
        _test_label_pos = _test_label_ifs.tellg();

        // 読み出しのためのインデックス作成：単なる整数型でOK(int)
        for (int i = 0; i < _number_of_test_data; i++)
        {
            _test_indices.push_back(i);
        }
    }

}

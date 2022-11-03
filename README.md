# MNIST_loader_sample

## 概要

C++のMNISTローダサンプルです。
主にEigenと合わせて使うことを前提にしています。

## 使い方

### 事前にやること
まず、`datasets/data`内のgzファイルは解凍してください(容量制限でアップロードできず)。

### 使用方法
MnistEigenDatasetというクラスが作成したローダです。

 
2. MnistEigenDatasetのインスタンス作成時には、コンストラクタの引数にバッチサイズを渡す。
3. 訓練データ読み出しは next_train メソッドを使用。読み出したいEigen::Matrixを引数にセットして使う。
   行列の形状は 
   - 画像：(バッチサイズ)×(784)
   - ラベル：(バッチサイズ)×(1) or (10)
4. テストデータ読み出しも同様(next_test)。
5. next_train, next_testには、one_hot_labelとnormalizeのフラグをセットできる。
   - one_hot_label：デフォルトはfalse → ラベルをone hot vectorにするか否かの設定
   - normalize：デフォルトはtrue → 画像データを正規化(0~1の範囲に)するか否かの設定

### サンプルコードの動かし方

1. インクルードパスには"include/"と"datasets/include"の両方を指定してください。
2. その上で"main/train_mnist_two_layer_net.cpp"をコンパイル。

### 動作環境
Windows10 WSL Ubuntu18.04  
コンパイラ g++

# MNIST_loader_sample

## 概要

C++のMNISTローダサンプルです。
主にEigenと合わせて使うことを前提にしています。

## 使い方

### 事前にやること
まず、`datasets/data`内のgzファイルは解凍してください(容量制限でアップロードできず)。

### 使用方法
MnistEigenDatasetというクラスが作成したローダです。

1. MnistEigenDatasetのインスタンス作成時には、コンストラクタの引数にバッチサイズを渡す。
2. 訓練データ読み出しは next_train メソッドを使用。読み出したいEigen::Matrixを引数にセットして使う。
   行列の形状は 
   - 画像：(バッチサイズ)×(784)
   - ラベル：(バッチサイズ)×(1) or (10)
3. テストデータ読み出しも同様(next_test)。
4. next_train, next_testには、one_hot_labelとrandom_loadのフラグをセットできる。
   - one_hot_label：デフォルトはfalse
   - random_load：デフォルトはtrue

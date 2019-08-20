# randomstring

擬似乱数を利用してランダムな文字列を作成する学習用のデモ

## 概要

```xorshift```・```RDRAND```・```std::random```・```CryptGenRandom```を使用してランダムな文字列を作成しようとしています。


## 使用法

Visual Studioはそのままソリューションを開いてください。

```g++ -std=c++11 -mrdrnd randomstring.cpp -o randomstring```

## 諸注意

- 学習用ですのでいろいろアラが有り、暗号用途には使えません。
- MinGWでは ```std::random_device``` にバグが存在します。回避策として ```CryptGenRandom``` を利用する ```my_random_device``` というものを作成してみました。
- Windows以外では ```CryptGenRandom``` を使えませんので、```my_random_device``` は ```std::random_device``` としました。
- 単純な剰余を使っている部分があります。この場合乱数に偏りが出ることが知られています。(modulo bias)。その場合は ```my_random_device``` のようなラッパークラスを用意し、```std::uniform_int_distribution``` を使うと良いでしょう。


## ライセンス

 MIT ライセンス

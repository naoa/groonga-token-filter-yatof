# Yet another token filter plugin for Groonga

## Token Filters

Groonga5.0.1以降で直る予定のパッチが適用されるまでは、オフラインインデックス構築時に適切にヒットしなくなる可能性があります。

### ``TokenFilterMaxLength``

検索時、追加時の両方で64バイトを超えるトークンを除去します。  
無駄な長いキーによる語彙表のキーサイズの逼迫を防ぐことができます。

環境変数``GRN_YATOF_MAX_TOKEN_LENGTH``でバイト数を変更することができます。

```bash
export GRN_YATOF_MAX_TOKEN_LENGTH=16
env
```

### ``TokenFilterMinLength``

検索時、追加時の両方で3バイト未満のトークンを除去します。  
頻出しやすい短い単語を除去することで検索速度の劣化を抑制することができます。

環境変数``GRN_YATOF_MIN_TOKEN_LENGTH``でバイト数を変更することができます。

### ``TokenFilterUnmaturedOne``

検索時、追加時の両方でNgramのNに満たないトークンであって1文字のトークンを除去します。
1文字での検索が不要な場合、転置索引の容量を削減することができます。

### ``TokenFilterTFLimit``

検索時、追加時の両方で同一文書中に含まれるトークン数が所定の閾値を超えたトークンを捨てます。  
テーブル``tf_limits``とカラム``tf_limit``を作ることによりトークンごとに上限値を設定することができます。
サイズの大きい文書において、頻出しすぎるトークンによるポスティングリストの長大化を抑制します。  
1文書ごとにハッシュ表でトークン数を数えているため、インデックス構築速度が結構劣化するとおもいます。

環境変数``GRN_YATOF_TF_LIMIT``で最大トークン数を``GRN_YATOF_TF_LIMIT_WORD_TABLE_NAME``でテーブル名を変更することができます。

テーブルにも環境変数にも設定がない場合、Groongaのデフォルトと同様に131071個の上限でトークンが捨てられます。

```bash
table_create tf_limits TABLE_HASH_KEY ShortText
[[0,0.0,0.0],true]
column_create tf_limits tf_limit COLUMN_SCALAR UInt32
[[0,0.0,0.0],true]
load --table tf_limits
[
{"_key": "a", "tf_limit": 3}
]
[[0,0.0,0.0],1]
tokenize TokenBigram "a a a a b b b b"--normalizer NormalizerAuto --token_filters TokenFilterTFLimit
[
  [
    0,
    0.0,
    0.0
  ],
  [
    {
      "value": "a",
      "position": 0
    },
    {
      "value": "a",
      "position": 1
    },
    {
      "value": "a",
      "position": 2
    },
    {
      "value": "b",
      "position": 4
    },
    {
      "value": "b",
      "position": 5
    },
    {
      "value": "b",
      "position": 6
    },
    {
      "value": "b",
      "position": 7
    }
  ]
]
```

### ``TokenFilterPhraseLimit``

検索時、追加時の両方で同一文書中に含まれる2トークンからなるフレーズが4096個を超えたトークンを捨てます。
サイズの大きい文書において、頻出しすぎるトークンによるポスティングリストの長大化
を抑制します。
1文書ごとにハッシュ表でトークン数を数えているため、インデックス構築速度が結構劣
化するとおもいます。

環境変数``GRN_YATOF_PHRASE_LIMIT``で最大フレーズ数を変更することができます。

### ``TokenFilterProlong``

検索時、追加時の両方で4文字以上の全角カタカナのみのトークンの末尾の長音記号を除去します。  

```bash
tokenize TokenDelimit   "フリーザー カー ブザー"   --token_filters TokenFilterProlong
[
  [
    0,
    0.0,
    0.0
  ],
  [
    {
      "value": "フリーザ",
      "position": 0
    },
    {
      "value": "カー",
      "position": 1
    },
    {
      "value": "ブザー",
      "position": 2
    }
  ]
]
```

### ``TokenFilterSymbol``

検索時、追加時の両方で記号のみのトークンを除去します。  

### ``TokenFilterDigit``

検索時、追加時の両方で数字のみのトークンを除去します。  

### ``TokenFilterIgnoreWord``

検索時、追加時の両方でテーブルのキーと一致するトークンを無視します。無視されたトークンは、positionを進めません。すなわち、無視されたトークンはないものとみなされ、そのトークンを取り除いたフレーズでもヒットするようになります。たとえば、以下の例では、"Hello World"でもヒットします。同一視できるフレーズが増える一方、誤ヒットにつながることもあるため、注意が必要です。  
あらかじめ除外対象の語句が格納されたテーブル``ignore_words``を作る必要があります。  
整合性を保つため、無視対象の語句を追加した場合は、インデックス再構築が必要です。

環境変数``GRN_YATOF_IGNORE_WORD_TABLE_NAME``でテーブルを変更することができます。

```bash
table_create ignore_words TABLE_HASH_KEY ShortText
[[0,0.0,0.0],true]
load --table ignore_words
[
{"_key": "and"}
]
[[0,0.0,0.0],1]
tokenize TokenBigram "Hello and World"   --normalizer NormalizerAuto   --token_filters TokenFilterIgnoreWord
[[0,0.0,0.0],[{"value":"hello","position":0},{"value":"world","position":1}]]
```

### ``TokenFilterRemoveWord``

検索時、追加時の両方でテーブルのキーと一致するトークンを除去します。除去されたトークンは、positionを進めます。すなわち、除去されたトークンは、他の除去トークンと同一視されるようになります。たとえば、以下の例では、"Hello and World"は、"Hello or World"でもヒットしますが、"Hello World"ではヒットしません。  
あらかじめ除外対象の語句が格納されたテーブル``remove_words``を作る必要があります。  
検索時、追加時の両方で除去されるため、転置索引のサイズを抑えることができます。しかし、整合性を保つため、除外対象の語句を追加した場合は、インデックス再構築が必要です。  
Groongaにバンドルされている``TokenFilterStopWord``は検索時のみ除外されるため、インデックス再構築は不要です。しかし、転置索引のサイズを抑えることはできません。

環境変数``GRN_YATOF_REMOVE_WORD_TABLE_NAME``でテーブルを変更することができます。

```bash
table_create remove_words TABLE_HASH_KEY ShortText
[[0,0.0,0.0],true]
load --table remove_words
[
{"_key": "and"},
{"_key": "or"}
]
[[0,0.0,0.0],2]
tokenize TokenBigram "Hello and World"   --normalizer NormalizerAuto   --token_filters TokenFilterRemoveWord
[[0,0.0,0.0],[{"value":"hello","position":0},{"value":"world","position":2}]]
```

### ``TokenFilterThroughWord``

検索時、追加時の両方でテーブルのキーと一致するトークンのみを通します。除去されたトークンは、positionを進めます。
あらかじめ除外対象の語句が格納されたテーブル``through_words``を作る必要があります。
検索時、追加時の両方で除去されるため、転置索引のサイズを抑えることができます。しかし、整合性を保つため、除外対象の語句を追加した場合は、インデックス再構築が必要です。

環境変数``GRN_YATOF_THROUGH_WORD_TABLE_NAME``でテーブルを変更することができます。

```bash
table_create through_words TABLE_HASH_KEY ShortText
[[0,0.0,0.0],true]
load --table through_words
[
{"_key": "hello"},
{"_key": "world"}
]
[[0,0.0,0.0],2]
tokenize TokenBigram "Hello and World"   --normalizer NormalizerAuto   --token_filters TokenFilterThroughWord
[[0,0.0,0.0],[{"value":"hello","position":0},{"value":"world","position":2}]]
```

### ``TokenFilterSynonym``

検索時、追加時の両方でテーブルのキーと一致するトークンを同義語に変換します。
あらかじめ変換対象の語句がキーに格納されたテーブル``synonyms``と変換後の語句が格納されたカラム``synonym``を作る必要があります。  
整合性を保つため、語句を追加した場合は、インデックス再構築が必要です。複数のワードに変換することはできません。

環境変数``GRN_YATOF_SYNONYM_TABLE_NAME``でテーブルを変更することができます。

```bash
table_create synonyms TABLE_HASH_KEY ShortText
[[0,0.0,0.0],true]
column_create synonyms synonym COLUMN_SCALAR ShortText
[[0,0.0,0.0],true]
load --table synonyms
[
{"_key": "senna", "synonym": "groonga"}
]
[[0,0.0,0.0],1]
tokenize TokenBigram "Hello Senna"   --normalizer NormalizerAuto   --token_filters TokenFilterSynonym
[[0,0.0,0.0],[{"value":"hello","position":0},{"value":"groonga","position":1}]]
```

## Install

Install ``groonga-token-filter-yatof`` package:

### CentOS

* CentOS6

```
% sudo yum localinstall -y http://packages.createfield.com/centos/6/groonga-token-filter-yatof-0.0.1-1.el6.x86_64.rpm
```

* CentOS7

```
% sudo yum localinstall -y http://packages.createfield.com/centos/7/groonga-token-filter-yatof-0.0.1-1.el7.centos.x86_64.rpm
```

### Fedora

未作成

* Fedora 20

```
% sudo yum localinstall -y http://packages.createfield.com/fedora/20/groonga-token-filter-yatof-0.0.1-1.fc20.x86_64.rpm
```

* Fedora 21

```
% sudo yum localinstall -y http://packages.createfield.com/fedora/21/groonga-token-filter-yatof-0.0.1-1.fc21.x86_64.rpm
```

### Debian GNU/LINUX

* wheezy

```
% wget http://packages.createfield.com/debian/wheezy/groonga-token-filter-yatof_0.0.1-1_amd64.deb
% sudo dpkg -i groonga-token-filter-yatof_0.0.1-1_amd64.deb
```

### Ubuntu

* precise

```
% wget http://packages.createfield.com/ubuntu/precise/groonga-token-filter-yatof_0.0.1-1_amd64.deb
% sudo dpkg -i groonga-token-filter-yatof_0.0.1-1_amd64.deb
```

* trusty

```
% wget http://packages.createfield.com/ubuntu/trusty/groonga-token-filter-yatof_0.0.1-1_amd64.deb
% sudo dpkg -i groonga-token-filter-yatof_0.0.1-1_amd64.deb
```

* utopic

```
% wget http://packages.createfield.com/ubuntu/utopic/groonga-token-filter-yatof_0.0.1-1_amd64.deb
% sudo dpkg -i groonga-token-filter-yatof_0.0.1-1_amd64.deb
```

### Source install

Build this tokenizer.

    % sh autogen.sh
    % ./configure
    % make
    % sudo make install

## Dependencies

* Groonga >= 4.0.7

Install ``groonga-devel`` in CentOS/Fedora. Install ``libgroonga-dev`` in Debian/Ubuntu.

See http://groonga.org/docs/install.html

## Usage

Firstly, register `token_filters/yatof`

Groonga:

    % groonga db
    > register token_filters/yatof
    > table_create Diaries TABLE_HASH_KEY INT32
    > column_create Diaries body COLUMN_SCALAR TEXT
    > table_create Terms TABLE_PAT_KEY ShortText \
    >   --default_tokenizer TokenBigram
    >   --token_filters TokenFilterSymbol,TokenFilterDigit
    > column_create Terms diaries_body COLUMN_INDEX|WITH_POSITION Diaries body

Mroonga:

    mysql> use db;
    mysql> select mroonga_command("register token_filters/yatof");
    mysql> CREATE TABLE `Diaries` (
        -> id INT NOT NULL,
        -> body TEXT NOT NULL,
        -> PRIMARY KEY (id) USING HASH,
        -> FULLTEXT INDEX (body) COMMENT 'token_filters "TokenFilterSymbol,TokenFilterDigit"'
        -> ) ENGINE=Mroonga DEFAULT CHARSET=utf8;

Rroonga:

    irb --simple-prompt -rubygems -rgroonga
    >> Groonga::Context.default_options = {:encoding => :utf8}   
    >> Groonga::Database.create(:path => "/tmp/db")
    >> Groonga::Plugin.register(:path => "token_filters/yatof")
    >> Groonga::Schema.create_table("Diaries",
    ?>                              :type => :hash,
    ?>                              :key_type => :integer32) do |table|
    ?>   table.text("body")
    >> end
    >> Groonga::Schema.create_table("Terms",
    ?>                              :type => :patricia_trie,
    ?>                              :normalizer => :NormalizerAuto,
    ?>                              :default_tokenizer => "TokenBigram",
    ?>                              :token_filters => "TokenFilterSymbol,TokenFilterDigit") do |table|
    ?>   table.index("Diaries.body")
    >> end
    
## Author

* Naoya Murakami <naoya@createfield.com>

## License

LGPL 2.1. See COPYING for details.

This program is the same license as Groonga.

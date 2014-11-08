# Yet another token filter plugin for Groonga

## Token Filters

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

### ``TokenFilterTFLimit``

検索時、追加時の両方で同一文書中に含まれるトークン数が65535を超えたトークンを捨てます。  
Groongaのデフォルトでは131071で捨てられます。  
サイズの大きい文書において、頻出しすぎるトークンによるポスティングリストの長大化を抑制します。

環境変数``GRN_YATOF_TOKEN_LIMIT``で最大トークン数を変更することができます。

### ``TokenFilterProlong``

検索時、追加時の両方で4文字以上のカタカナのみのトークンの末尾の長音記号を除去します。  
例：データー → データ

## Install

Install ``groonga-token-filter-yatof`` package:

未作成

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

* jessie

```
% wget http://packages.createfield.com/debian/jessie/groonga-token-filter-yatof_0.0.1-1_amd64.deb
% sudo dpkg -i groonga-token-filter-yatof_0.0.1-1_amd64.deb
```

* sid

```
% wget http://packages.createfield.com/debian/sid/groonga-token-filter-yatof_0.0.1-1_amd64.deb
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
    >   --token_filters TokenFilterMaxLength
    > column_create Terms diaries_body COLUMN_INDEX|WITH_POSITION Diaries body

Mroonga:

    mysql> use db;
    mysql> select mroonga_command("register token_filters/yatof");
    mysql> CREATE TABLE `Diaries` (
        -> id INT NOT NULL,
        -> body TEXT NOT NULL,
        -> PRIMARY KEY (id) USING HASH,
        -> FULLTEXT INDEX (body) COMMENT 'token_filters "TokenFilterMaxLength"'
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
    ?>                              :token_filters => "TokenFilterMaxLength") do |table|
    ?>   table.index("Diaries.body")
    >> end
    
## Author

* Naoya Murakami <naoya@createfield.com>

## License

LGPL 2.1. See COPYING for details.

This program is the same license as Groonga.

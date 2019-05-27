# Simple C Book Downloader

### A pdf/djvu/epub downloader using [Library Genesis](http://gen.lib.rus.ec/).

#### Installation

`git clone https://github.com/josecleiton/book-downloader.git`

`cd book-downloader/src`

`sudo make install`

`scbd -h`

#### Usage

Required arguments:

        -s               Set the string search (usually author/book name)


Optional arguments:

        -h               Show this help message

        -o               Set another sort order { id | author | title | publisher | year | pages | filesize | extension }

        -h               Set another sort mode { ASC[endant] | DESC[endant] }

        -b               Set another reference folder (where .bib will be downloaded)

        -d               Set another book folder

        -v               Verbose mode

usage: `-s STR [ -b STR ] [ -d STR ] [ -v ]`

Don't forget to read the `LICENSE` file.

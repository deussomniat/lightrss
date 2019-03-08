# lightrss

## description
A simple qt based rss feed reader

## license
* Source code is licensed under the GPL
* Rss icon provided under the LGPL by [Everaldo Coelho](http://www.everaldo.com/)
* All other icons provided under the GPL by [Andy Fitzsimon](http://www.fitzsimon.com.au/)

## required
* qt 5.12

## build
* qmake
* make

## run
* ./lightrss

## itunes
lightrss supports itunes podcast urls in the format below.

https://itunes.apple.com/us/podcast/mysterious-universe/id329937558

You can enter those urls in the text box, click the + icon and
it will automatically be converted to the original rss url.

## what ISN'T lightrss?
* it is not a download manager
* it does not track read/unread articles
* it does not notify you of new articles/podcasts
* it does not perform scheduled updates

## what is lightrss?
After the removal of live bookmarks from firefox I was left without
an rss reader. I only use a handful of rss feeds and the majority
of them are podcasts. I prefer to use firefox as my download manager
and I loved the simplicity of viewing feeds with live bookmarks.

After exploring a few rss reader options on linux I eventually
decided that a new application was needed. I use kde so the app was
developed with the qt framework. It is built and tested ONLY on
linux. Qt is a cross-platform library so it MAY work on other
platforms but I have no plans to support them. If you discover that
it compiles and runs successfully on Mac/Win I'd love to hear about
it.

This application is designed for the sole purpose of displaying rss
items and links. If you click on a link it will open in your system's
default browser. All podcast enclosures (e.g., mp3) will also open
in an external program so you can use whichever download manager you
prefer. lightrss is not intended to replace a more robust rss application
like akregator or gpodder. It is for users who prefer to manually initiate
the rss updates, manage downloads with an external program and do not need
to be reminded of which items you've viewed.

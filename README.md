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
* ./lightrss http://host/feed1.xml http://host/feed2.xml

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

## templates
This application supports the use of templates to map xml elements and
attributes from the source feed to the various qt widgets in lightrss.

Each template begins with the root element.

    <template>
    </template>

You then specify whether you're extracting the values from the
channel element or the item elements.

    <template>
      <channel>
      </channel>
      <item>
      </item>
    </template>

At this point you can begin adding map elements to specify the src
values and destination widgets. The src attribute points to either
the text content of an element or an attribute value from the original
rss feed. A child node is specified with a period. An attribute is
specified with the @ symbol. All supported dst values are listed in
the example below.

    <template>
      <channel>
        <map src="title" dst="feed_title"/>
        <map src="image.url" dst="feed_image"/>
        <map src="itunes:image@href" dst="feed_image"/>
      </channel>
      <item>
        <map src="title" dst="item_title"/>
        <map src="pubDate" dst="item_date"/>
        <map src="link" dst="item_webpage"/>
        <map src="enclosure@url" dst="item_download"/>
        <map src="enclosure@length" dst="item_size"/>
        <map src="itunes:duration" dst="item_duration"/>
        <map src="content:encoded" dst="item_description"/>
        <map src="description" dst="item_description"/>
      </item>
    </template>

You can specify multiple options for the same dst value. If the first
option does not contain a value then the second option will be used and
so on. In the example above there are 2 options for the item_description.
If the content:encoded element is not found in the source feed then the
description element will be used.

For an example, let's say that you want to create a template for itunes
podcast feeds. You could eliminate the image.url map and you could replace
the content:encoded map with an itunes:summary map.

    <template>
      <channel>
        <map src="title" dst="feed_title"/>
        <map src="itunes:image@href" dst="feed_image"/>
      </channel>
      <item>
        <map src="title" dst="item_title"/>
        <map src="pubDate" dst="item_date"/>
        <map src="link" dst="item_webpage"/>
        <map src="enclosure@url" dst="item_download"/>
        <map src="enclosure@length" dst="item_size"/>
        <map src="itunes:duration" dst="item_duration"/>
        <map src="itunes:summary" dst="item_description"/>
        <map src="description" dst="item_description"/>
      </item>
    </template>

Once you've created the template you can save the file in your ~/.lightrss/templates
folder. Then specify the template filename in your ~/.lightrss/catalog.xml file.

    <tpl>my_itunes_template.xml</tpl>

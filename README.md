# tagger

Set IDx tags for a wide range media formats ([see here](https://taglib.org/api/classTagLib_1_1Tag.html)).
Use this to modify your tracks e.g. to display them correctly on your phone (with artist, genre, etc.)

```
Available options:
  -h [ --help ]         produce this help message
  -i [ --file ] arg     specify one or more files to operate on
  -g [ --get ] arg      get IDx tag information
  -s [ --set ] arg      set IDx tag information
  -c [ --clear ]        clear all tags
 ```
 
 ## Examples:
 ```
 tagger track.mp3 --set 'artist=ACDC; genre=Rock; title=Back in Black' # set some tags
 tagger track.mp3				# view all tags
 tagger -c track.mp3			# clear all tags
 tagger -g 'artist' track.mp3	# show 'artist' tag
 tagger -s 'genre="Rock" "Hard rock"' -i track.mp3	# set multiple values if the format allows it
 ```
 

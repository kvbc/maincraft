; Texture Atlas generator
; Requires ImageMagick, see github.com/kvbc/tsgen
magick montage -tile 1x -geometry +0+0 -resize 256 grass.jpg grass_top.jpg grass_bottom.jpg texatlas.jpg
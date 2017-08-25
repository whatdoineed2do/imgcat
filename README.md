# imgcat / imgprextr

## `imgcat`

This is a simple multi-threaded command line util that will recursively find image and video files, generating thumbnails for producing an index.html to allow easy browsing.  Generated index will link to found files and can (using EXIF data) group equivalant files (ie a RAW file and a generated jpg) together under one image.

The intended use is for cataloguing large collections of images (particularly RAW files) to provide a single index in the form of a index.html; avoid the need for traditional file browsers to attempt to generate it's own thumbnails.  This is particularly suited for DVD backups where the thumbnails (ie Windows .thumbs/ dirs) cannot be generated, with the generated index and catalogue being genreated and burned to the same backup.  Instead of browsing the individual directories on the DVD (that would potentially cause redundant reads of each file to generate/extract thumbnails to display), we can browse the `index.html` and click through to required files.

For (Nikon NEF/Canon CR2/Fuji RAF) RAW files, the tool will extract the largest embedded thumbnails from the RAW file to generate thumbnail.

To recognise other RAW formats, supported by Exiv2, update `main` and `DFLT_EXTNS` along with `ImgExifParser.cc` and the block that assigns `data.type = ImgData::EMBD_PREVIEW`

Sample RAW files available https://rawsamples.ch/index.php/en/

## `imgprextr`

Command line util to extract largest thumbnail from (Nikon) RAW files;  adds functionality not available in `exiv2` or `exiftool` in that this tool can extract the thumbnail and perform colour space conversions (Adobe to sRGB) and also resizing of the images before writing to disk

## Dependancies
Require `ImageMagick`, `exiv2` and `SampleICC` (local copy available) and `ffmpegthumbnailer` development libraries

ImageGPSBound
=============
Extract Images By GPS Metadata

### What
The `bound` command copies specific images between two directories if their GPS
metadata indicates a location within a defined bounding rectangle. The exif.h
library is a separate project on GitHub.

### Usage
Simply `make` to generate the `bound` command from source. The parameters to the
bound command are, in this order, the source directory, the destination directory, the
top left corner latitude, the top left corner longitude, the bottom right corner
latitude, and the bottom right corner longitude. All coordinate values are decimal
values with negative signs in front as necessary.

### Example
If you wanted to copy all images from /src to /dest whose GPS coordinates fall in
the bounding rectangle between (38.5 N, 122 W) and (37.5 N, 121 W), you would issue the
command `bound /src /dest 38.5 -122 37.5 -121`.

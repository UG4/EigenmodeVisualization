import os
import sys


print(str(sys.argv))
os.system("ffmpeg -f image2 -framerate 15 -i " + sys.argv[1] + "/screenshot_%01d.png -vcodec mpeg4 -y " + sys.argv[2])

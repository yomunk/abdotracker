import cv2
import numpy as np
from matplotlib import pylab as pyl

print cv2.__version__


cv2.namedWindow("window")
mothstream = cv2.VideoCapture(0)
pyl.ion()

if mothstream.isOpened():
    cam, mothframe = mothstream.read();
else:
    cam = False


def searchforgreatest(array):
    maxarea = 0
    max = 0
    currentarea = 0
    for i in range(0, len(array)):
        currentarea = cv2.contourArea(array[i])
        if currentarea > maxarea:
            max = i
            maxarea = currentarea
    return max
            
            
        

while True:
    
    cam, mothframe = mothstream.read()
    mothframe1 = cv2.blur(mothframe, (11,11))
    mothframe2 = cv2.threshold(mothframe1, 120, 255, cv2.THRESH_BINARY)[1]
    mothframe3 = cv2.Canny(mothframe2, 100, 100)
    contours,hierarchy = cv2.findContours(mothframe3, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
    if len(contours) > 0:
        largestcontour = searchforgreatest(contours)
        circlecenter, radius = cv2.minEnclosingCircle(contours[largestcontour])
        fixedcenter = (int(circlecenter[0]), int(circlecenter[1]))
        fixedradius = int(radius)
        cv2.circle(mothframe, fixedcenter , fixedradius,255,0)

    cv2.imshow("window", mothframe)
        
  
    pyl.subplot(121),pyl.imshow(mothframe3, cmap = 'gray')
    pyl.title("image"), pyl.xticks([]), pyl.yticks([])
    pyl.draw();
    
    key = cv2.waitKey(10)
    if key == 27:
        break

cv2.destroyWindow("window")
pyl.ioff()
pyl.close('all')
    
    
   
    

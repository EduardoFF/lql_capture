import numpy as np
import matplotlib.pyplot as plt
from scipy.misc import imread
#import matplotlib.cbook as cbook
import sys
import scipy.interpolate
from matplotlib.mlab import griddata
import Image
import numpy.ma as ma
import matplotlib.cm as colormaps


def RandomD():
    return random.random()*2.0


def normal_interp(x, y, a, xi, yi):
    rbf = scipy.interpolate.Rbf(x, y, a)
    ai = rbf(xi, yi)
    return ai

def rescaled_interp(x, y, a, xi, yi):
    a_rescaled = (a - a.min()) / a.ptp()
    ai = normal_interp(x, y, a_rescaled, xi, yi)
    ai = a.ptp() * ai + a.min()
    return ai



def testInterpolate(xs,ys,rs):

    X = np.array(xs)
    Y = np.array(ys)
    A = np.array(rs)

    minx=np.amin(xs)
    miny=np.amin(ys)
    maxx=np.amax(xs)
    maxy=np.amax(ys)
    cmap = colormaps.get_cmap('Greys_r')

    xi = np.linspace(minx,maxx,200)
    yi = np.linspace(miny,maxy,200)

    xi, yi = np.meshgrid(xi, yi)
    inte = 'nn'
    zi = griddata(X,Y,A,xi,yi,interp=inte)
    zm = ma.masked_where(np.isnan(zi),zi)
    print zm
#    zm = zm
    z_min, z_max = np.amin(zm), np.amax(zm)
#    z_min, z_max = -np.abs(zm).max(), np.abs(zm).max()
    plt.imshow(zm, cmap='jet', vmin=z_min, vmax=z_max,
           extent=[xi.min(), xi.max(), yi.min(), yi.max()],
               interpolation='nearest', origin='lower',alpha=0.5)
#    plt.pcolormesh(xi,yi,zm,cmap=cmap,vmin=0.0, vmax=1.0)

    

mapfile = sys.argv[1]
img = Image.open(mapfile)
print img.size
(w,h) = img.size


ratio = min(1000.0/w, 1000.0/h)
newsize=(int(w*ratio), int(h*ratio))
rsize= img.resize(newsize,Image.ANTIALIAS)
rsizeArr = np.asarray(rsize)


np.random.seed(int(sys.argv[2]))
minx = 0000
maxx = 1000
miny = 0
maxy = 1000
n=100
x = np.random.uniform(minx,maxx,n)
y = np.random.uniform(miny,maxy,n)
#v = np.random.normal(0.0,16.0,n+4)
v = np.random.uniform(0.0,16.0,n+4)
x = np.append(x,[minx,minx,maxx,maxx])
y = np.append(y,[miny,maxy,miny,maxy])

testInterpolate(x,y,v)
imgplot = plt.imshow(rsizeArr)
#datafile = cbook.get_sample_data('lena.)



#plt.scatter(x,y,zorder=1)
#plt.imshow(img, zorder=0, extent=[0.5, 8.0, 1.0, 7.0])
plt.show()

# Saliency-object-detection

## Dependency
``CMAKE 3.11.3``
``OPENCV 3.5.1_5``

### For Mac
```
brew install cmake
brew install opencv
```

## Directory
```
.  
├── main      
    ├── dip.cpp                   # our main source code    
    ├── CMakeList.txt             # cmake configuration, which includes some options to print processing image   
    ├── compile.sh                # compile   
    ├── build.sh                  # cmake
    ├── segementation             # segementation algorithm implemented by davidstutz[1]
├── include                       # implement region based saliency    
    ├── saliencyMap.hpp                
    ├── saliencyMap.cpp    
├── test_img                      # Some test image    
├── result_img                    # Result from test image   

├── Global Contrast Based Salient Region Detection.pdf   # Our slides in pdf format    
├── Global Contrast Based Salient Region Detection.pptx  # Our slides in pptx format    
├── Team4_ProposalReport.pdf                             # Our proposal    
└── README.md  
```
[1] [davidstutz's segmentation implementation](https://github.com/davidstutz/graph-based-image-segmentation)   


## Usage

All operations are under ``main/``
```
cd main/
```
### Compile

```
sh build.sh
sh compile.sh  
```
### Execute
```
./DIP [Path/To/Image]  
```
### Play with parameter
```
vim ../saliencyMap.cpp
sh compile.sh
./DIP [Path/To/Image]
```
![](https://github.com/tall15421542/Saliency-object-detection/blob/master/img/parameter.png)  

All parameters are specified in [Global Contrast Based Salient Region Detection.pdf](https://github.com/tall15421542/Saliency-object-detection/blob/master/img/parameter.png)

### Options

There are some options for showing addtional image during the process.
```
vim CMakeLists.txt /* uncomment your option */
```

![alt](https://github.com/tall15421542/Saliency-object-detection/blob/master/img/options.png)

## To be fix
 
Some of the images may lead to segmentation fault due to our function ``find_largest_connect_region`` can not handle the cases when the connected region grows too large.

## Result

 ![](https://github.com/tall15421542/Saliency-object-detection/blob/master/result_img/plane.png) 
 ---
 ![](https://github.com/tall15421542/Saliency-object-detection/blob/master/result_img/bear2.png)
 ---
 ![](https://github.com/tall15421542/Saliency-object-detection/blob/master/result_img/pika.png)
 
 ## Reference paper
[Ming-Ming Cheng, Niloy J. Mitra, Xiaolei Huang, Philip H. S. Torr, and Shi-Min Hu, “ Global Contrast Based Salient Region Detection IEEE TRAN PATTERN ANALYSIS AND MACHINE INTELLIGENCE,
VOL. 37, NO. 3, p.569-p.582, MARCH 2015](https://ieeexplore.ieee.org/document/6871397/)


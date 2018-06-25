# Saliency-object-detection
## Implement paper
[Ming-Ming Cheng, Niloy J. Mitra, Xiaolei Huang, Philip H. S. Torr, and Shi-Min Hu, “ Global Contrast Based Salient Region Detection IEEE TRAN PATTERN ANALYSIS AND MACHINE INTELLIGENCE,
VOL. 37, NO. 3, p.569-p.582, MARCH 2015](https://ieeexplore.ieee.org/document/6871397/)

## Dependency
``CMAKE 3.11.3``
``OPENCV 3.5.1_5``

## Directory
```
.  
├── main      
    ├── dip.cpp                   # our main source code    
    ├── CMakeList.txt             # cmake configuration, which includes some options to print processing image   
    ├── compile.sh                # compile   
    ├── build.sh                  # cmake
    ├── segementation             # segementation algorithm implement by davidstutz
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
ref: [davidstutz's segmentation implement](https://github.com/davidstutz/graph-based-image-segmentation)   


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
![](https://github.com/tall15421542/Saliency-object-detection/blob/master/img/%E8%9E%A2%E5%B9%95%E5%BF%AB%E7%85%A7%202018-06-25%20%E4%B8%8B%E5%8D%8810.04.33.png)  

All parameters are specified in [Global Contrast Based Salient Region Detection.pdf](https://github.com/tall15421542/Saliency-object-detection/blob/master/Global%20Contrast%20Based%20Salient%20Region%20Detection.pdf)

### Options

There are some options, which are for showing the processing image during the process.
```
vim CMakeLists.txt /* uncomment your option */
```

![alt](https://github.com/tall15421542/Saliency-object-detection/blob/master/img/%E8%9E%A2%E5%B9%95%E5%BF%AB%E7%85%A7%202018-06-25%20%E4%B8%8B%E5%8D%8810.22.23.png)

## To be fix
 
Some of the images may lead to segmentation fault due to our function ``find_largest_connect_region`` can not handle the cases when the connected region grows too large.

## Result

 ![](https://github.com/tall15421542/Saliency-object-detection/blob/master/result_img/plane.png) 
 ---
 ![](https://github.com/tall15421542/Saliency-object-detection/blob/master/result_img/bear2.png)
 ---
 ![](https://github.com/tall15421542/Saliency-object-detection/blob/master/result_img/pika.png)

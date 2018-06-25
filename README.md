# Saliency-object-detection

## Dependency
``CMAKE 3.11.3``
``OPENCV 3.5.1_5``

## Usage
```
cd main  
mkdir build  
cd build  
cmake ..  
cd ../  
sh compile.sh  
./DIP [path/to/image]  
```
## Option

There are some options, which are for showing the processing image during the process.
```
cd main  
vi CMakeLists.txt
```

![alt](https://github.com/tall15421542/Saliency-object-detection/blob/master/img/%E8%9E%A2%E5%B9%95%E5%BF%AB%E7%85%A7%202018-06-25%20%E4%B8%8B%E5%8D%886.26.50.png)

## To be fix
 
Some of the images may lead to segmentation fault due to our function ``find_largest_connect_region`` can not handle the cases when the connected region grows too large.
 

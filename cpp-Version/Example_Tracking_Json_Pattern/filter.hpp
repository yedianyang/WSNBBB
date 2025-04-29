#ifndef FILTER_HPP
#define FILTER_HPP

int lowPassFilter(int currentValue, int newValue, float alpha);
int adaptiveLowPassFilter(int currentValue, int newValue, float baseAlpha);

#endif
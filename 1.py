import requests
import redis
import lxml.html
from pymongo import MongoClient
from selenium import webdriver
import time

"""获取网页源代码"""
# get()得到一个response对象
# content属性来显示bytes型网页
# decode()将bytes型网页源代码解码为字符串型的源代码，因为bytes型数据下，中文无法正常显示
# decode()的参数可以为'UTF-8'(默认), 'GBK', 'GB2312', 'GB18030'，以中文能正常显示为准
# code = requests.get('http://m.sohu.com/a/216890590_328665').content.decode()

"""得到数据"""
# selector = lxml.html.fromstring(code)
# data_1 = selector.xpath('//div[@class="display-content"]')[0]
# data_2 = selector.xpath('//div[@class="hidden-content"]')
# print(data_2)
# article_1 = data_1.xpath('string(.)')
# article_2 = data_2.xpath('string(.)')
# # article = article_1 + article_2
# print(article_1)
# print(article_2)


# """第二种方法(chrome版本暂时不支持)"""
driver = webdriver.Chrome('./chromedriver')
driver.get('http://m.sohu.com/a/216890590_328665')
time.sleep(10)
html = driver.page_source
print(html)
from selenium import webdriver
import time


def get_command_url(html):
    selector = lxml.html.fromstring(html)
    title_sorted = selector.xpath(//td/strong)
    print(title_sorted)



driver = webdriver.Chrome('./chromedriver')
driver.get('https://www.runoob.com/linux/linux-command-manual.html')
time.sleep(10)
html = driver.page_source

get_command_url(html)

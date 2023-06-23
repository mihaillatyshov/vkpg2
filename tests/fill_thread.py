import time
import unittest

import requests

host = "http://localhost:8080/api"

class TestThread(unittest.TestCase):
    def threadCreate(self, slug, json, resStatus = 201):
        with self.subTest(slug=slug, json=json, resStatus=resStatus):
            resp = requests.post(f"{host}/forum/{slug}/create", json=json)
            print(resp.status_code, ":\n    ", resp.json())
            self.assertEqual(resp.status_code, resStatus)
            
    def threadsGet(self, slug, params="", resStatus = 200):
        with self.subTest(slug=slug, resStatus=resStatus):
            resp = requests.get(f"{host}/forum/{slug}/threads{params}")
            print(resp.status_code, ":\n    ", resp.json())
            self.assertEqual(resp.status_code, resStatus)
            
    def threadUpdate(self, slug, json, resStatus = 200):
        with self.subTest(slug=slug, json=json, resStatus=resStatus):
            resp = requests.post(f"{host}/thread/{slug}/details", json=json)
            print(resp.status_code, ":\n    ", resp.json())
            self.assertEqual(resp.status_code, resStatus)
        
    def threadGet(self, slug, resStatus = 200):
        with self.subTest(slug=slug, resStatus=resStatus):
            resp = requests.get(f"{host}/thread/{slug}/details")
            print(resp.status_code, ":\n    ", resp.json())
            self.assertEqual(resp.status_code, resStatus)


    def test_threadCreate(self):
        print()
        self.threadCreate(f"forum-slug-1", {
            "title": f"Title 1",
            "author": f"lm1",
            "message": "Some message 1"
        })
        time.sleep(0.1)
        
        
        self.threadCreate(f"forum-slug-1", {
            "title": f"Title 2",
            "author": f"lm1",
            "message": "Some message 2"
        })
        time.sleep(0.1)
        
        self.threadCreate(f"forum-slug-1", {
            "title": f"Title OLD",
            "author": f"lm1",
            "message": "Some OLD message",
            "created": "2017-01-01T00:00:00.000Z"
        })
        time.sleep(0.1)
        
        self.threadCreate(f"forum-slug-2", {
            "title": f"Title 2",
            "author": f"lm1",
            "message": "Some message 2",
            "created": "2017-01-01T00:00:00.000Z"
        })
        time.sleep(0.1)
            
        self.threadCreate(f"forum-slug-1", {
            "title": f"Title 2",
            "author": f"lm4",
            "slug": f"thread-slug-4",
            "message": "Some message 2"
        })
        
        self.threadCreate(f"forum-slug-1", {
            "title": f"Title 2",
            "author": f"lm4",
            "slug": f"thread-slug-4",
            "message": "Some message 2"
        }, 409)

        self.threadCreate(f"forum-slug-3", {
            "title": f"Title 2",
            "author": f"lm1222",
            "message": "Some message 3"
        }, 404)
        
        self.threadCreate(f"forum-slug-331", {
            "title": f"Title 2",
            "author": f"lm1",
            "message": "Some message 3"
        }, 404)
        
        
    def test_threadsGet(self):
        print()
        
        self.threadsGet(f"forum-slug-1")
        
        self.threadsGet(f"forum-slug-1", "?desc=false")
        self.threadsGet(f"forum-slug-1", "?desc=true")
        self.threadsGet(f"forum-slug-1", "?limit=1")
        
        self.threadsGet(f"forum-slug-1", "?limit=1&since=2018-01-01T00:00:00.000Z")
        self.threadsGet(f"forum-slug-1", "?limit=1&since=2018-01-01T00:00:00.000Z&desc=true")
        
        self.threadsGet(f"forum-slug-14242341", resStatus=404)
        
        
    def test_threadUpdate(self):
        print()
        
        self.threadUpdate(f"thread-slug-4", {"title": "Updated thread-slug-4 title", "message": "Updated message 1"}) 
        self.threadUpdate(f"1", {"title": "Updated id 1 title", "message": "Updated message 2"}) 
        
        self.threadUpdate(f"thread-slug-1", {"title": "Updated thread-slug-4 title", "message": "Updated message 1"}, 404) 
        
        
    def test_threadGet(self):
        print()
        
        self.threadGet(f"thread-slug-4") 
        self.threadGet(f"1") 
        
        self.threadGet(f"thread-slug-1", 404) 
        
        
        
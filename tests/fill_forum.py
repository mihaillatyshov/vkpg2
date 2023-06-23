import unittest

import requests

host = "http://localhost:8080/api"

class TestForum(unittest.TestCase):
    def forumCreate(self, json, resStatus = 201):
        with self.subTest(json=json, resStatus=resStatus):
            resp = requests.post(f"{host}/forum/create", json=json)
            print(resp.status_code, ":   ", resp.json())
            self.assertEqual(resp.status_code, resStatus)
        
    def forumDetails(self, slug, resStatus = 200):
        with self.subTest(slug=slug, resStatus=resStatus):
            resp = requests.get(f"{host}/forum/{slug}/details")
            print(resp.status_code, ":   ", resp.json())
            self.assertEqual(resp.status_code, resStatus)
            
    def forumUsers(self, slug, resStatus = 200):
        with self.subTest(slug=slug, resStatus=resStatus):
            resp = requests.get(f"{host}/forum/{slug}/users")
            print(resp.status_code, ":   ", resp.json())
            self.assertEqual(resp.status_code, resStatus)
    

    def test_forumCreate(self):
        print()
        for i in [1, 2, 3, 4]:
            self.forumCreate({
                "title": f"Title {i}",
                "user": f"lm{i}",
                "slug": f"forum-slug-{i}"
            })
            
        self.forumCreate({
            "title": f"Title 2",
            "user": f"lm1",
            "slug": f"forum-slug-4"
        }, 409)

        self.forumCreate({
            "title": f"Title 2",
            "user": f"lm1222",
            "slug": f"forum-slug-5"
        }, 404)
        
        
    def test_forumDetails(self):
        print()
        self.forumDetails("forum-slug-1");
        self.forumDetails("forum-slug-5", 404);
        
    def test_forumUsers(self):
        print()
        self.forumUsers("forum-slug-1");
        self.forumUsers("forum-slug-5", 404);
        
        
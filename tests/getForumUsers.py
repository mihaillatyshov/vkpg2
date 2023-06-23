import unittest

import requests

host = "http://localhost:8080/api"


class TestForumUsers(unittest.TestCase):
    def forumUsers(self, slug, params="", resStatus = 200):
        with self.subTest(slug=slug, resStatus=resStatus):
            resp = requests.get(f"{host}/forum/{slug}/users{params}")
            print(resp.status_code, ":   ", resp.json())
            self.assertEqual(resp.status_code, resStatus)
        

    def test_forumUsers(self):
        print()
        self.forumUsers(f"forum-slug-1")
        
        self.forumUsers(f"forum-slug-1", "?desc=false")
        self.forumUsers(f"forum-slug-1", "?desc=true")
        self.forumUsers(f"forum-slug-1", "?limit=1")
        
        self.forumUsers(f"forum-slug-1", "?limit=1&since=lm1")
        self.forumUsers(f"forum-slug-1", "?limit=1&since=lm1&desc=true")
        

        self.forumUsers(f"forum-slug-14242341", resStatus=404)
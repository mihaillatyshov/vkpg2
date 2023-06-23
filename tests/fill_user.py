import unittest

import requests

host = "http://localhost:8080/api"

class TestUser(unittest.TestCase):
    def userCreate(self, id, json, resStatus = 201):
        with self.subTest(id=id, json=json, resStatus=resStatus):
            resp = requests.post(f"{host}/user/lm{id}/create", json=json)
            # print(resp.status_code, ":   ", resp.json())
            self.assertEqual(resp.status_code, resStatus)
            
    def userProfileGet(self, id, resStatus = 200): 
        with self.subTest(id=id, resStatus=resStatus):
            resp = requests.get(f"{host}/user/lm{id}/profile")
            # print(resp.status_code, ":   ", resp.json())
            self.assertEqual(resp.status_code, resStatus)
            
    def userProfilePost(self, id, json, resStatus = 200):
        with self.subTest(id=id, json=json, resStatus=resStatus):
            resp = requests.post(f"{host}/user/lm{id}/profile", json=json)
            # print(resp.status_code, ":   ", resp.json())
            self.assertEqual(resp.status_code, resStatus)
    
    
    def test_userCreate(self):
        print()
        for i in [1, 2, 3, 4]:
            self.userCreate(i, {
                "email": f"lmail{i}@mail.ru",
                "about": f"some about {i}",
                "fullname": f"Name {i}"
            })
            
        self.userCreate(5, {
            "email": f"lmail5@mail.ru",
            "fullname": f"Name 5"
        })    

        self.userCreate(3, {
                "email": f"lmail5@mail.ru",
                "about": f"some about",
                "fullname": f"Name"
            }, 409)
        

    def test_userProfileGet(self):
        print()
        self.userProfileGet(1);
        self.userProfileGet(8, 404);


    def test_userProfilePost(self):
        print()
        self.userProfilePost(1, {
            "email": f"lmail01@mail.ru",
            "about": f"some about 01",
            "fullname": f"Name 01"
        })
        
        self.userProfilePost(2, {
            "email": f"lmail3@mail.ru",
            "about": f"some about 01",
            "fullname": f"Name 01"
        }, 409)

        self.userProfilePost(1222, {
            "email": f"lmail3@mail.ru",
            "about": f"some about 01",
            "fullname": f"Name 01"
        }, 404)

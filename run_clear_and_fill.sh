#!/bin/bash
psql 'postgresql://postgres:postgres@localhost:5432/forum' -f ./postgresql/schemas/db_1.sql

# cd tests
# echo " "
# python3 -m unittest -v fill_user.py fill_forum.py fill_thread.py getForumUsers.py
# echo " "


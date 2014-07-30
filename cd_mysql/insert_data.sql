--  Delete existing data
delete from track;
delete from cd;
delete from artist;

-- Now the data inserts

-- First the artist (or group) tables
insert into artist(id, name) values(1, 'Pink Floyd');
insert into artist(id, name) values(2, 'Guns & Roses');
insert into artist(id, name) values(3, 'Li Zhi');
insert into artist(id, name) values(4, 'Liu Dong Ming');

-- Then the cd table
insert into cd(id, title, artist_id, catalogue) values(1, 'The Wall', 1, 'B000024D4P');
insert into cd(id, title, artist_id, catalogue) values(2, 'Use Your Illusion II', 2, 'B000024D4S');
insert into cd(id, title, artist_id, catalogue) values(3, 'Fan Gao Xian Sheng', 3, 'B000024ZC1');
insert into cd(id, title, artist_id, catalogue) values(4, 'F', 3, 'B000024ZC2');
insert into cd(id, title, artist_id, catalogue) values(5, 'Liu Er De Ba Xi', 4, 'B000024LE1');
insert into cd(id, title, artist_id, catalogue) values(6, 'Da Di Mi Cang', 4, 'B000024LE2');

-- Populate the tracks
insert into track(cd_id, track_id, title) values(1, 1, 'Another Brick in the Wall');
insert into track(cd_id, track_id, title) values(2, 1, 'Donot Cry');
insert into track(cd_id, track_id, title) values(3, 1, 'Fan Gao Xian Sheng');
insert into track(cd_id, track_id, title) values(3, 2, 'Chun Mo De Nan Fang Cheng Shi');
insert into track(cd_id, track_id, title) values(3, 3, 'Xie');
insert into track(cd_id, track_id, title) values(4, 1, 'Hang Zhou');
insert into track(cd_id, track_id, title) values(4, 2, 'Xia Yu');
insert into track(cd_id, track_id, title) values(4, 3, 'Shan Yin Lu De Xia Tian');
insert into track(cd_id, track_id, title) values(5, 1, 'Xi Bei Pian Bei');
insert into track(cd_id, track_id, title) values(5, 2, 'Gen Ju Zhen Ren Zhen Shi Gai Bian');
insert into track(cd_id, track_id, title) values(6, 1, 'Zai Song Chen Zhang Pu');


CREATE DATABASE IF NOT EXISTS qqchat CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

USE qqchat;

DROP TABLE IF EXISTS users;
CREATE TABLE users (
    user_id INT PRIMARY KEY AUTO_INCREMENT,
    username VARCHAR(64) UNIQUE NOT NULL,
    password_hash VARCHAR(64) NOT NULL,
    avatar_url VARCHAR(256),
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_username (username)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS friends;
CREATE TABLE friends (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    friend_id INT NOT NULL,
    status INT NOT NULL DEFAULT 1,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_user_friend (user_id, friend_id),
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    FOREIGN KEY (friend_id) REFERENCES users(user_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS messages;
CREATE TABLE messages (
    msg_id BIGINT PRIMARY KEY AUTO_INCREMENT,
    from_id INT NOT NULL,
    to_id INT NOT NULL,
    content TEXT NOT NULL,
    timestamp DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_from_to (from_id, to_id),
    INDEX idx_to_from (to_id, from_id),
    FOREIGN KEY (from_id) REFERENCES users(user_id) ON DELETE CASCADE,
    FOREIGN KEY (to_id) REFERENCES users(user_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

INSERT INTO users (username, password_hash) VALUES 
('admin',    'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f'),
('user1',   'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f'),
('user2',   'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f'),
('test',    'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f');

INSERT INTO friends (user_id, friend_id) VALUES 
(1, 2),
(1, 3),
(1, 4),
(2, 1),
(2, 3),
(3, 1),
(3, 2),
(4, 1);

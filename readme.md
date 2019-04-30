Pour compiler le programme : 
	gcc -o huffman huffman.c

Pour exécuter le programme :
	Compresser > .../huffman fichier_entrée -c fichier_sortie
	Décompresser > .../huffman fichier_entrée -d fichier_sortie

Remarque :
	Il est possible de pouvoir compresser n'importe quel type de fichier, et même des fichiers de très grande taille
	Le dossier ex contient plusieurs exemple de fichier compressé et décompressé

Conseil : Vous pouvez utiliser la commande md5sum pour vérifier si des fichiers sont identique

